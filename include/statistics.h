#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdbool.h>

// Structure for tracking file information
typedef struct FileInfo {
    char* filename;
    long size_bytes;
    int line_count;
} FileInfo;

// Structure for tracking errors
typedef struct Error {
    char* filename;
    int line_number;
    char* variable_name;
    struct Error* next;
} Error;

// Structure for tracking included files
typedef struct IncludedFile {
    FileInfo info;
    struct IncludedFile* next;
} IncludedFile;

// Statistics structure
typedef struct {
    int variables_checked;
    int error_count;
    Error* errors;
    int comment_lines_removed;
    int included_file_count;
    IncludedFile* included_files;
    FileInfo input_file_info;
    long output_size_bytes;
    int output_line_count;
} Stats;

/**
 * Initializes the statistics structure.
 * 
 * @param stats Pointer to statistics structure
 */
void init_stats(Stats* stats);

/**
 * Frees resources allocated for statistics.
 * 
 * @param stats Pointer to statistics structure
 */
void free_stats(Stats* stats);

/**
 * Adds an included file to the statistics.
 * 
 * @param stats Pointer to statistics structure
 * @param filename Name of the included file
 */
void add_included_file(Stats* stats, const char* filename);

/**
 * Prints the processing statistics.
 * 
 * @param stats Pointer
 * @param stats Pointer to statistics structure
 */
void print_stats(Stats* stats);

#endif /* STATISTICS_H */