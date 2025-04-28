#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "preprocessor.h"
#include "fileutils.h"
#include "statistics.h"

void print_usage(char* program_name) {
    fprintf(stderr, "Usage: %s [-i|--in] input_file [-o|--out output_file] [-v|--verbose]\n", program_name);
    fprintf(stderr, "  -i, --in     : input file (required if not provided as first argument)\n");
    fprintf(stderr, "  -o, --out    : output file (optional, stdout if not specified)\n");
    fprintf(stderr, "  -v, --verbose: show processing statistics\n");
}

int main(int argc, char *argv[]) {
    char *input_file = NULL;
    char *output_file = NULL;
    bool verbose = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--in") == 0) {
            if (i + 1 < argc) {
                input_file = argv[++i];
            } else {
                fprintf(stderr, "Error: Input file is missing after %s option\n", argv[i]);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else if (strncmp(argv[i], "--in=", 5) == 0) {
            input_file = argv[i] + 5;
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--out") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: Output file is missing after %s option\n", argv[i]);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else if (strncmp(argv[i], "--out=", 6) == 0) {
            output_file = argv[i] + 6;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (input_file == NULL) {
            // First non-option argument is treated as input file
            input_file = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown option or argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    
    // Check if input file is provided
    if (input_file == NULL) {
        fprintf(stderr, "Error: Input file is required\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    // Initialize statistics
    Stats stats;
    init_stats(&stats);
    
    // Process the input file
    char *processed_code = preprocess_file(input_file, &stats);
    if (processed_code == NULL) {
        fprintf(stderr, "Error: Failed to preprocess the input file\n");
        return EXIT_FAILURE;
    }
    
    // Output the result
    if (output_file != NULL) {
        // Write to the specified output file
        if (!write_to_file(output_file, processed_code, &stats)) {
            fprintf(stderr, "Error: Failed to write to output file: %s\n", output_file);
            free(processed_code);
            free_stats(&stats);
            return EXIT_FAILURE;
        }
    } else {
        // Write to stdout
        printf("%s", processed_code);
    }
    
    // Print statistics if verbose mode is enabled
    if (verbose) {
        print_stats(&stats);
    }
    
    // Clean up
    free(processed_code);
    free_stats(&stats);
    
    return EXIT_SUCCESS;
}