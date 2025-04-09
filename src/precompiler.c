#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "precompiler.h"

// funzione per stampare le statistiche
void print_stats(const precompiler_stats_t *stats) { 
    fprintf(stderr, "Variabili controllate: %d\n", stats->num_variables);
    fprintf(stderr, "Errori rilevati: %d\n", stats->num_errors);
    fprintf(stderr, "Commenti eliminati: %d\n", stats->num_comments_removed);
    fprintf(stderr, "File inclusi: %d\n", stats->num_files_included);
    fprintf(stderr, "Righe input: %d - Byte: %ld\n", stats->input_lines, stats->input_size);
    fprintf(stderr, "Righe output: %d - Byte: %ld\n", stats->output_lines, stats->output_size);
}