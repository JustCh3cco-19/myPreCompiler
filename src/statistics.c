#include "statistics.h"
#include "fileutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_stats(Stats* stats) {
    stats->variables_checked = 0;
    stats->error_count = 0;
    stats->errors = NULL;
    stats->comment_lines_removed = 0;
    stats->included_file_count = 0;
    stats->included_files = NULL;
    stats->input_file_info.filename = NULL;
    stats->input_file_info.size_bytes = 0;
    stats->input_file_info.line_count = 0;
    stats->output_size_bytes = 0;
    stats->output_line_count = 0;
}

void free_stats(Stats* stats) {
    // Free input file info
    if (stats->input_file_info.filename != NULL) {
        free(stats->input_file_info.filename);
    }
    
    // Free errors
    Error* current_error = stats->errors;
    while (current_error != NULL) {
        Error* next_error = current_error->next;
        free(current_error->filename);
        free(current_error->variable_name);
        free(current_error);
        current_error = next_error;
    }
    
    // Free included files
    IncludedFile* current_include = stats->included_files;
    while (current_include != NULL) {
        IncludedFile* next_include = current_include->next;
        free(current_include->info.filename);
        free(current_include);
        current_include = next_include;
    }
}

void add_included_file(Stats* stats, const char* filename) {
    // Create new included file entry
    IncludedFile* new_file = (IncludedFile*)malloc(sizeof(IncludedFile));
    if (new_file == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for included file\n");
        return;
    }
    
    // Allocate and copy filename
    new_file->info.filename = strdup(filename);
    if (new_file->info.filename == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for included filename\n");
        free(new_file);
        return;
    }
    
    // Get file size and line count
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open included file for stats: %s\n", filename);
        free(new_file->info.filename);
        free(new_file);
        return;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fprintf(stderr, "Error: Failed to determine size of included file: %s\n", filename);
        fclose(file);
        free(new_file->info.filename);
        free(new_file);
        return;
    }
    
    new_file->info.size_bytes = file_size;
    
    // Count lines
    int line_count = 0;
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            line_count++;
        }
    }
    
    // Check if last line doesn't end with newline
    if (file_size > 0) {
        fseek(file, -1, SEEK_END);
        if (fgetc(file) != '\n') {
            line_count++;
        }
    }
    
    new_file->info.line_count = line_count;
    fclose(file);
    
    // Add to linked list
    new_file->next = NULL;
    if (stats->included_files == NULL) {
        stats->included_files = new_file;
    } else {
        IncludedFile* last = stats->included_files;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new_file;
    }
    
    // Increment counter
    stats->included_file_count++;
}

void print_stats(Stats* stats) {
    printf("\nProcessing Statistics:\n");
    printf("=====================\n");
    
    // Variables and errors
    printf("Variables checked: %d\n", stats->variables_checked);
    printf("Errors detected: %d\n", stats->error_count);
    
    // List errors
    if (stats->error_count > 0) {
        printf("Error details:\n");
        Error* current = stats->errors;
        while (current != NULL) {
            printf("  - File '%s', Line %d: Invalid variable name '%s'\n", 
                   current->filename, current->line_number, current->variable_name);
            current = current->next;
        }
    }
    
    // Comments
    printf("Comment lines removed: %d\n", stats->comment_lines_removed);
    
    // Included files
    printf("Files included: %d\n", stats->included_file_count);
    
    // Input file info
    if (stats->input_file_info.filename != NULL) {
        printf("Input file: '%s', Size: %ld bytes, Lines: %d\n", 
               stats->input_file_info.filename, 
               stats->input_file_info.size_bytes, 
               stats->input_file_info.line_count);
    }
    
    // Included files info
    if (stats->included_file_count > 0) {
        printf("Included files details:\n");
        IncludedFile* current = stats->included_files;
        while (current != NULL) {
            printf("  - File '%s', Size: %ld bytes, Lines: %d\n", 
                   current->info.filename, 
                   current->info.size_bytes, 
                   current->info.line_count);
            current = current->next;
        }
    }
    
    // Output
    printf("Output: Size: %ld bytes, Lines: %d\n", 
           stats->output_size_bytes, 
           stats->output_line_count);
}