#ifndef MYPRECOMPILER_H
#define MYPRECOMPILER_H

#include <stdio.h>
#include <stdbool.h>

// --- Strutture Dati ---

// Struttura per memorizzare i dettagli di un errore di identificatore
typedef struct {
    char* filename;         // Nome file allocato dinamicamente
    int line_number;
    char* identifier_name;  // Nome identificatore allocato dinamicamente
} IdentifierError;

// Struttura per memorizzare le statistiche di un file
typedef struct {
    char* filename;         // Nome file allocato dinamicamente
    long size_bytes;
    int lines;
} FileStats;

// Struttura principale per le statistiche di elaborazione
typedef struct {
    int vars_checked;
    int errors_found;
    IdentifierError* errors; // Array allocato dinamicamente
    int error_capacity;      // Capacità attuale dell'array errors

    int comments_removed;    // Numero di righe *contenenti* commenti eliminati
    int includes_processed;

    FileStats input_file_stats;
    FileStats* included_files_stats; // Array allocato dinamicamente
    int included_files_capacity;     // Capacità attuale dell'array included_files_stats

    int output_lines;
    long output_size_bytes; // Tracciato durante la scrittura

    bool verbose; // Flag per l'output delle statistiche
} ProcessingStats;

// --- Dichiarazioni Funzioni ---

// Funzioni in utils.c
void init_stats(ProcessingStats* stats, bool verbose_mode);
void add_identifier_error(ProcessingStats* stats, const char* filename, int line, const char* identifier);
void add_included_file_stats(ProcessingStats* stats, const char* filename, long size, int lines);
void print_stats(const ProcessingStats* stats, FILE* stream); // stream può essere stdout o stderr
void free_stats(ProcessingStats* stats);
bool is_valid_c_identifier(const char* str);
char* extract_include_filename(const char* line); // Estrae il nome file da #include "..."

// Funzioni in preprocessor.c
int process_c_file(const char* input_filename, FILE* out_stream, ProcessingStats* stats, int depth); // Aggiunto depth per evitare include infiniti


#endif // MYPRECOMPILER_H