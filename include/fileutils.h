#ifndef FILEUTILS_H
#define FILEUTILS_H

#include "statistics.h"

/**
 * Reads the contents of a file into a dynamically allocated string.
 * Updates the statistics with file information.
 * 
 * @param filename The name of the file to read
 * @param stats Pointer to statistics structure
 * @return A dynamically allocated string containing the file contents, or NULL on error
 */
char* read_file(const char* filename, Stats* stats);

/**
 * Writes a string to a file.
 * Updates the statistics with output file information.
 * 
 * @param filename The name of the file to write
 * @param content The string to write to the file
 * @param stats Pointer to statistics structure
 * @return true on success, false on failure
 */
bool write_to_file(const char* filename, const char* content, Stats* stats);

#endif /* FILEUTILS_H */