#include "preprocessor.h"
#include "fileutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Forward declarations of helper functions
static bool is_valid_identifier(const char* identifier);
static void add_error(Stats* stats, const char* filename, int line_number, const char* variable_name);
static int count_lines(const char* text, int up_to_position);

char* preprocess_file(const char* filename, Stats* stats) {
    // Read the input file
    char* code = read_file(filename, stats);
    if (code == NULL) {
        return NULL;
    }

    // Resolve #include directives
    char* code_with_includes = resolve_includes(code, filename, stats);
    free(code);
    if (code_with_includes == NULL) {
        return NULL;
    }

    // Check for invalid variable names
    check_variable_names(code_with_includes, filename, stats);

    // Remove comments
    char* final_code = remove_comments(code_with_includes, stats);
    free(code_with_includes);
    
    return final_code;
}

char* resolve_includes(const char* code, const char* base_filename, Stats* stats) {
    if (code == NULL) return NULL;

    // Initial buffer size and allocation
    size_t buffer_size = strlen(code) * 2; // Start with twice the original size
    char* result = (char*)malloc(buffer_size);
    if (result == NULL) {
        fprintf(stderr, "Error: Memory allocation failed in resolve_includes\n");
        return NULL;
    }
    result[0] = '\0';
    
    const char* pos = code;
    size_t result_len = 0;
    
    while (*pos != '\0') {
        // Look for #include directive
        if (strncmp(pos, "#include", 8) == 0 && (isspace(*(pos+8)) || *(pos+8) == '\"' || *(pos+8) == '<')) {
            // Skip "#include" and any whitespace
            pos += 8;
            while (isspace(*pos)) pos++;
            
            // Check if it's a local include with quotes
            if (*pos == '\"') {
                pos++; // Skip opening quote
                
                // Extract the filename
                const char* filename_start = pos;
                while (*pos != '\"' && *pos != '\0' && *pos != '\n') pos++;
                
                if (*pos == '\"') {
                    // Found complete include filename
                    size_t filename_len = pos - filename_start;
                    char* include_filename = (char*)malloc(filename_len + 1);
                    if (include_filename == NULL) {
                        fprintf(stderr, "Error: Memory allocation failed for include filename\n");
                        free(result);
                        return NULL;
                    }
                    
                    strncpy(include_filename, filename_start, filename_len);
                    include_filename[filename_len] = '\0';
                    
                    // Read the included file
                    char* included_code = read_file(include_filename, stats);
                    if (included_code != NULL) {
                        // Add to included files statistics
                        add_included_file(stats, include_filename);
                        
                        // Process includes recursively in the included file
                        char* processed_include = resolve_includes(included_code, include_filename, stats);
                        free(included_code);
                        
                        if (processed_include != NULL) {
                            // Calculate new required buffer size
                            size_t include_len = strlen(processed_include);
                            size_t required_size = result_len + include_len + 1;
                            
                            // Resize result buffer if needed
                            if (required_size > buffer_size) {
                                buffer_size = required_size * 2; // Double the required size
                                char* new_result = (char*)realloc(result, buffer_size);
                                if (new_result == NULL) {
                                    fprintf(stderr, "Error: Memory reallocation failed in resolve_includes\n");
                                    free(result);
                                    free(processed_include);
                                    free(include_filename);
                                    return NULL;
                                }
                                result = new_result;
                            }
                            
                            // Append processed include content
                            strcpy(result + result_len, processed_include);
                            result_len += include_len;
                            
                            free(processed_include);
                        }
                    } else {
                        fprintf(stderr, "Warning: Failed to include file: %s\n", include_filename);
                    }
                    
                    free(include_filename);
                    pos++; // Skip closing quote
                } else {
                    // Incomplete include directive, keep as is
                    size_t directive_len = pos - (filename_start - 9); // Include "#include "
                    
                    // Resize result buffer if needed
                    size_t required_size = result_len + directive_len + 1;
                    if (required_size > buffer_size) {
                        buffer_size = required_size * 2;
                        char* new_result = (char*)realloc(result, buffer_size);
                        if (new_result == NULL) {
                            fprintf(stderr, "Error: Memory reallocation failed in resolve_includes\n");
                            free(result);
                            return NULL;
                        }
                        result = new_result;
                    }
                    
                    // Copy incomplete directive
                    strncpy(result + result_len, pos - directive_len, directive_len);
                    result_len += directive_len;
                    result[result_len] = '\0';
                }
            } else {
                // System include with < > brackets - keep as is
                const char* directive_start = pos - 8; // Include "#include"
                while (*pos != '\n' && *pos != '\0') pos++;
                
                size_t directive_len = pos - directive_start;
                
                // Resize result buffer if needed
                size_t required_size = result_len + directive_len + 1;
                if (required_size > buffer_size) {
                    buffer_size = required_size * 2;
                    char* new_result = (char*)realloc(result, buffer_size);
                    if (new_result == NULL) {
                        fprintf(stderr, "Error: Memory reallocation failed in resolve_includes\n");
                        free(result);
                        return NULL;
                    }
                    result = new_result;
                }
                
                // Copy system include directive
                strncpy(result + result_len, directive_start, directive_len);
                result_len += directive_len;
                result[result_len] = '\0';
            }
        } else {
            // Regular character, copy to result
            // Resize result buffer if needed
            if (result_len + 2 > buffer_size) {
                buffer_size *= 2;
                char* new_result = (char*)realloc(result, buffer_size);
                if (new_result == NULL) {
                    fprintf(stderr, "Error: Memory reallocation failed in resolve_includes\n");
                    free(result);
                    return NULL;
                }
                result = new_result;
            }
            
            // Copy character
            result[result_len++] = *pos++;
            result[result_len] = '\0';
        }
    }
    
    return result;
}

void check_variable_names(const char* code, const char* filename, Stats* stats) {
    if (code == NULL) return;
    
    const char* pos = code;
    
    // Skip to variable declarations (before or at the beginning of main)
    while (*pos != '\0') {
        // Look for global variables before main
        if (isalpha(*pos) || *pos == '_') {
            // Potential type name (for global variable)
            // Skip to the identifier
            while (*pos != '\0' && !isspace(*pos)) pos++;
            while (*pos != '\0' && isspace(*pos)) pos++;
            
            if (*pos != '\0') {
                // Potential variable name
                const char* var_start = pos;
                
                // Find the end of the identifier
                while (*pos != '\0' && (isalnum(*pos) || *pos == '_')) pos++;
                
                // Check if it's a valid variable declaration
                if (*pos == ';' || *pos == ',' || *pos == '=' || isspace(*pos)) {
                    // Extract variable name
                    size_t var_len = pos - var_start;
                    if (var_len > 0) {
                        char* var_name = (char*)malloc(var_len + 1);
                        if (var_name != NULL) {
                            strncpy(var_name, var_start, var_len);
                            var_name[var_len] = '\0';
                            
                            // Check if it's a valid identifier
                            stats->variables_checked++;
                            if (!is_valid_identifier(var_name)) {
                                // Invalid identifier
                                int line_number = count_lines(code, var_start - code);
                                add_error(stats, filename, line_number, var_name);
                            }
                            
                            free(var_name);
                        }
                    }
                }
            }
        } else if (strncmp(pos, "int main", 8) == 0) {
            // Found main function, now look for local variables
            // Skip to opening brace
            while (*pos != '\0' && *pos != '{') pos++;
            if (*pos == '{') pos++;
            
            // Process local variable declarations
            while (*pos != '\0') {
                // Skip whitespace
                while (*pos != '\0' && isspace(*pos)) pos++;
                
                // Check if we've found a type name for a local variable
                if (isalpha(*pos) || *pos == '_') {
                    // Skip to the identifier
                    while (*pos != '\0' && !isspace(*pos)) pos++;
                    if (*pos == '\0') break;
                    
                    while (*pos != '\0' && isspace(*pos)) pos++;
                    if (*pos == '\0') break;
                    
                    // Process variable name(s)
                    bool continue_vars = true;
                    while (continue_vars) {
                        const char* var_start = pos;
                        
                        // Find the end of the identifier
                        while (*pos != '\0' && (isalnum(*pos) || *pos == '_')) pos++;
                        
                        // Extract variable name
                        size_t var_len = pos - var_start;
                        if (var_len > 0) {
                            char* var_name = (char*)malloc(var_len + 1);
                            if (var_name != NULL) {
                                strncpy(var_name, var_start, var_len);
                                var_name[var_len] = '\0';
                                
                                // Check if it's a valid identifier
                                stats->variables_checked++;
                                if (!is_valid_identifier(var_name)) {
                                    // Invalid identifier
                                    int line_number = count_lines(code, var_start - code);
                                    add_error(stats, filename, line_number, var_name);
                                }
                                
                                free(var_name);
                            }
                        }
                        
                        // Skip to next character
                        while (*pos != '\0' && isspace(*pos)) pos++;
                        
                        // Check if there are more variables in this declaration
                        if (*pos == ',') {
                            pos++; // Skip comma
                            // Skip whitespace
                            while (*pos != '\0' && isspace(*pos)) pos++;
                        } else if (*pos == ';') {
                            // End of declaration
                            pos++;
                            continue_vars = false;
                        } else {
                            // Unexpected character or assignment
                            // Skip to the end of this declaration
                            while (*pos != '\0' && *pos != ';') pos++;
                            if (*pos == ';') pos++;
                            continue_vars = false;
                        }
                    }
                } else {
                    // Not a variable declaration, we've processed all local variables
                    break;
                }
            }
            
            // Done processing local variables
            break;
        }
        
        // Move to next character
        if (*pos != '\0') pos++;
    }
}

char* remove_comments(const char* code, Stats* stats) {
    if (code == NULL) return NULL;
    
    size_t len = strlen(code);
    char* result = (char*)malloc(len + 1);
    if (result == NULL) {
        fprintf(stderr, "Error: Memory allocation failed in remove_comments\n");
        return NULL;
    }
    
    size_t i = 0, j = 0;
    bool in_string = false;
    bool in_char = false;
    bool in_line_comment = false;
    bool in_block_comment = false;
    
    while (i < len) {
        if (in_line_comment) {
            // In a line comment, look for the end (newline)
            if (code[i] == '\n') {
                result[j++] = code[i]; // Keep the newline
                in_line_comment = false;
            }
            i++;
            stats->comment_lines_removed++;
        } else if (in_block_comment) {
            // In a block comment, look for the end (*/)
            if (code[i] == '*' && i + 1 < len && code[i + 1] == '/') {
                i += 2; // Skip */
                in_block_comment = false;
                stats->comment_lines_removed++;
            } else {
                if (code[i] == '\n') {
                    result[j++] = code[i]; // Keep the newline
                    stats->comment_lines_removed++;
                }
                i++;
            }
        } else if (in_string) {
            // In a string literal, look for the end or escape sequences
            if (code[i] == '\\' && i + 1 < len) {
                // Copy escape sequence
                result[j++] = code[i++];
                result[j++] = code[i++];
            } else if (code[i] == '\"') {
                // End of string
                result[j++] = code[i++];
                in_string = false;
            } else {
                // Regular character in string
                result[j++] = code[i++];
            }
        } else if (in_char) {
            // In a character literal, look for the end or escape sequences
            if (code[i] == '\\' && i + 1 < len) {
                // Copy escape sequence
                result[j++] = code[i++];
                result[j++] = code[i++];
            } else if (code[i] == '\'') {
                // End of character literal
                result[j++] = code[i++];
                in_char = false;
            } else {
                // Regular character in character literal
                result[j++] = code[i++];
            }
        } else {
            // Not in a comment, string, or character literal
            if (code[i] == '\"') {
                // Start of string
                result[j++] = code[i++];
                in_string = true;
            } else if (code[i] == '\'') {
                // Start of character literal
                result[j++] = code[i++];
                in_char = true;
            } else if (code[i] == '/' && i + 1 < len) {
                if (code[i + 1] == '/') {
                    // Start of line comment
                    in_line_comment = true;
                    i += 2;
                } else if (code[i + 1] == '*') {
                    // Start of block comment
                    in_block_comment = true;
                    i += 2;
                } else {
                    // Division operator
                    result[j++] = code[i++];
                }
            } else {
                // Regular character
                result[j++] = code[i++];
            }
        }
    }
    
    // Null-terminate the result
    result[j] = '\0';
    
    return result;
}

// Helper function to check if an identifier is valid
static bool is_valid_identifier(const char* identifier) {
    // First character must be a letter or underscore
    if (!isalpha(*identifier) && *identifier != '_') {
        return false;
    }
    
    // Remaining characters must be letters, digits, or underscores
    const char* p = identifier + 1;
    while (*p) {
        if (!isalnum(*p) && *p != '_') {
            return false;
        }
        p++;
    }
    
    return true;
}

// Helper function to add an error to statistics
static void add_error(Stats* stats, const char* filename, int line_number, const char* variable_name) {
    // Allocate new error
    Error* error = (Error*)malloc(sizeof(Error));
    if (error == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for error tracking\n");
        return;
    }
    
    // Fill error information
    error->filename = strdup(filename);
    error->line_number = line_number;
    error->variable_name = strdup(variable_name);
    error->next = NULL;
    
    if (error->filename == NULL || error->variable_name == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for error tracking strings\n");
        if (error->filename) free(error->filename);
        if (error->variable_name) free(error->variable_name);
        free(error);
        return;
    }
    
    // Add to linked list
    if (stats->errors == NULL) {
        stats->errors = error;
    } else {
        Error* last = stats->errors;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = error;
    }
    
    // Increment error count
    stats->error_count++;
}

// Helper function to count lines up to a specific position
static int count_lines(const char* text, int up_to_position) {
    int lines = 1;
    for (int i = 0; i < up_to_position; i++) {
        if (text[i] == '\n') {
            lines++;
        }
    }
    return lines;
}