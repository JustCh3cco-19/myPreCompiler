#pragma once

#include <stdbool.h>

/**
 * @brief  myPreCompiler data types
\**********************************************************************/

typedef struct {
    char *input_file; // file da mandare in input
    char *output_file; // file su stdout
    bool valido; // controlla se l'input sia valido o meno
    bool verbose; // output verbose dove vengono mostrate tutte le statistiche necessarie
} config_t;

typedef struct 
{
    /* data */ 
    int num_variables; // numero di variabili
    int num_errors; // numero di errori
    int num_comments_removed; // numero di commenti rimossi
    int num_files_included; // numero di file inclusi
    long input_size; // size dell'input
    int input_lines; // righe in input
    long output_size; // size dell'output
    int output_lines; // righe in output
} precompiler_stats_t;

/**
 * @brief  myPreCompiler public functions
\**********************************************************************/
config_t parsing_argomenti(int argc, char *argv[]); // funzione che prende in input il file da analizzare e precompilare
char *preprocessing_file(const char *input_file, precompiler_stats_t *stats); // preprocessing del file in input
void print_stats(const precompiler_stats_t *stats); // stampo su stdout le statische in modalità verbose
                        // risolvere gli include


