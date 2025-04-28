#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "statistics.h"

/**
 * Preprocesses a C file:
 * - Resolves #include directives
 * - Checks for invalid variable names
 * - Removes comments
 * 
 * @param filename The name of the file to preprocess
 * @param stats Pointer to statistics structure
 * @return A dynamically allocated string containing the preprocessed code, or NULL on error
 */
char* preprocess_file(const char* filename, Stats* stats);

/**
 * Resolves #include directives in the given code.
 * 
 * @param code The C code to process
 * @param base_filename The name of the file being processed (for error reporting)
 * @param stats Pointer to statistics structure
 * @return A dynamically allocated string with includes resolved, or NULL on error
 */
char* resolve_includes(const char* code, const char* base_filename, Stats* stats);

/**
 * Checks for invalid variable names in the code.
 * 
 * @param code The C code to check
 * @param filename The name of the file being checked (for error reporting)
 * @param stats Pointer to statistics structure
 */
void check_variable_names(const char* code, const char* filename, Stats* stats);

/**
 * Removes all comments from the given code.
 * 
 * @param code The C code to process
 * @param stats Pointer to statistics structure
 * @return A dynamically allocated string with comments removed, or NULL on error
 */
char* remove_comments(const char* code, Stats* stats);

#endif /* PREPROCESSOR_H */