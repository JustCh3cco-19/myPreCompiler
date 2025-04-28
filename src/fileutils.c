#include "fileutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_file(const char* filename, Stats* stats) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open file: %s\n", filename);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fprintf(stderr, "Error: Failed to determine size of file: %s\n", filename);
        fclose(file);
        return NULL;
    }
    
    // Allocate buffer
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for file: %s\n", filename);
        fclose(file);
        return NULL;
    }
    
    // Read file content
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read < (size_t)file_size) {
        fprintf(stderr, "Error: Failed to read full content of file: %s\n", filename);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    // Null-terminate the buffer
    buffer[file_size] = '\0';
    
    // Update statistics if this is the input file
    if (stats->input_file_info.filename == NULL) {
        stats->input_file_info.filename = strdup(filename);
        stats->input_file_info.size_bytes = file_size;
        
        // Count lines
        int line_count = 0;
        for (long i = 0; i < file_size; i++) {
            if (buffer[i] == '\n') {
                line_count++;
            }
        }
        // Account for last line without newline
        if (file_size > 0 && buffer[file_size - 1] != '\n') {
            line_count++;
        }
        stats->input_file_info.line_count = line_count;
    }
    
    fclose(file);
    return buffer;
}

bool write_to_file(const char* filename, const char* content, Stats* stats) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open file for writing: %s\n", filename);
        return false;
    }
    
    size_t content_len = strlen(content);
    size_t bytes_written = fwrite(content, 1, content_len, file);
    
    if (bytes_written < content_len) {
        fprintf(stderr, "Error: Failed to write full content to file: %s\n", filename);
        fclose(file);
        return false;
    }
    
    fclose(file);
    
    // Update output statistics
    stats->output_size_bytes = content_len;
    
    // Count lines in output
    int line_count = 0;
    for (size_t i = 0; i < content_len; i++) {
        if (content[i] == '\n') {
            line_count++;
        }
    }
    // Account for last line without newline
    if (content_len > 0 && content[content_len - 1] != '\n') {
        line_count++;
    }
    stats->output_line_count = line_count;
    
    return true;
}