#pragma once

#include <stdbool.h>
#include <stddef.h>

/* -------------------------------------------------------------
 *  Configuration collected from command line
 * -----------------------------------------------------------*/
typedef struct {
    char *input_file;   /*  -i / --in  (mandatory)             */
    char *output_file;  /*  -o / --out (optional, stdout else) */
    bool  verbose;      /*  -v / --verbose                     */
    bool  valid;        /*  parsed ok?                         */
} config_t;

/* -------------------------------------------------------------
 *  Processing statistics (updated incrementally)
 * -----------------------------------------------------------*/
typedef struct {
    int   num_variables;        /* identifiers checked            */
    int   num_errors;           /* invalid identifiers found      */
    int   num_comments_removed; /* total comment blocks removed   */
    int   num_files_included;   /* #include files brought in      */
    long  input_size;           /* original INPUT bytes           */
    int   input_lines;          /* original INPUT lines           */
    long  output_size;          /* final OUTPUT bytes             */
    int   output_lines;         /* final OUTPUT lines             */
} precompiler_stats_t;

/* ================  Public API  =============================*/

config_t parsing_arguments(int argc, char *argv[]);
char    *preprocessing_file(const char *input_file, precompiler_stats_t *stats);
char    *resolve_includes (const char *code, precompiler_stats_t *stats);
char    *remove_comments  (const char *code, precompiler_stats_t *stats);
void     validate_identifiers(const char *code, precompiler_stats_t *stats, const char *filename);
void     handle_error(const char *message, const char *filename, bool fatal);
void     print_stats(const precompiler_stats_t *stats);