#ifndef MYPRECOMPILER_H
#define MYPRECOMPILER_H

#include <stdio.h>
#include <stdbool.h>

// =======================
// Strutture Dati Principali
// =======================

/**
 * Rappresenta un errore relativo a un identificatore non valido.
 * Contiene il nome del file, il numero di riga e il nome dell'identificatore incriminato.
 * Tutti i campi stringa sono allocati dinamicamente e vanno liberati.
 */
typedef struct {
    char* filename;         // Nome del file dove si trova l'errore
    int line_number;        // Numero di riga dell'errore
    char* identifier_name;  // Nome dell'identificatore errato
} IdentifierError;

/**
 * Raccoglie statistiche su un singolo file processato.
 * Utile sia per il file principale che per i file inclusi.
 */
typedef struct {
    char* filename;         // Nome del file (allocato dinamicamente)
    long size_bytes;        // Dimensione del file in byte
    int lines;              // Numero di righe del file
} FileStats;

/**
 * Struttura principale che raccoglie tutte le statistiche di elaborazione.
 * Tiene traccia di errori, commenti rimossi, file inclusi, output generato e modalità verbosa.
 */
typedef struct {
    int vars_checked;               // Numero di variabili analizzate
    int errors_found;               // Numero totale di errori trovati
    IdentifierError* errors;        // Array dinamico di errori sugli identificatori
    int error_capacity;             // Capacità attuale dell'array errors

    int comments_removed;           // Numero di righe contenenti commenti rimossi
    int includes_processed;         // Numero di direttive #include processate

    FileStats input_file_stats;     // Statistiche sul file di input principale
    FileStats* included_files_stats;// Array dinamico di statistiche sui file inclusi
    int included_files_capacity;    // Capacità attuale dell'array included_files_stats

    int output_lines;               // Numero di righe scritte in output
    long output_size_bytes;         // Numero di byte scritti in output

    bool verbose;                   // Flag per abilitare la stampa delle statistiche
} ProcessingStats;

// =======================
// Dichiarazioni delle Funzioni di Utility
// =======================

// Inizializza la struttura delle statistiche
void init_stats(ProcessingStats* stats, bool verbose_mode);

// Aggiunge un errore relativo a un identificatore non valido
void add_identifier_error(ProcessingStats* stats, const char* filename, int line, const char* identifier);

// Aggiunge le statistiche di un file incluso
void add_included_file_stats(ProcessingStats* stats, const char* filename, long size, int lines);

// Stampa le statistiche di elaborazione su uno stream (stdout o stderr)
void print_stats(const ProcessingStats* stats, FILE* stream);

// Libera tutta la memoria allocata nella struttura delle statistiche
void free_stats(ProcessingStats* stats);

// Verifica se una stringa è un identificatore C valido
bool is_valid_c_identifier(const char* str);

// Estrae il nome del file da una direttiva #include "..."
char* extract_include_filename(const char* line);

// =======================
// Dichiarazioni Funzioni di Preprocessing
// =======================

// Processa ricorsivamente un file C, rimuove commenti, gestisce #include e aggiorna le statistiche.
// Il parametro depth serve a evitare inclusioni ricorsive infinite.
int process_c_file(const char* input_filename, FILE* out_stream, ProcessingStats* stats, int depth);

#endif // MYPRECOMPILER_H