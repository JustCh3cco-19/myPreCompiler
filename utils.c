#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include "myPreCompiler.h"

/**
 * Inizializza la struttura delle statistiche di elaborazione.
 * Imposta tutti i contatori a zero e i puntatori a NULL.
 * @param stats Puntatore alla struttura da inizializzare.
 * @param verbose_mode Se true, abilita la stampa delle statistiche.
 */
void init_stats(ProcessingStats* stats, bool verbose_mode) {
    stats->vars_checked = 0;
    stats->errors_found = 0;
    stats->errors = NULL;
    stats->error_capacity = 0;
    stats->comments_removed = 0;
    stats->includes_processed = 0;

    // Inizializza le statistiche del file di input
    stats->input_file_stats.filename = NULL;
    stats->input_file_stats.size_bytes = 0;
    stats->input_file_stats.lines = 0;

    stats->included_files_stats = NULL;
    stats->included_files_capacity = 0;

    stats->output_lines = 0;
    stats->output_size_bytes = 0;

    stats->verbose = verbose_mode;
}

/**
 * Registra un errore relativo a un identificatore non valido.
 * Rialloca l'array dinamico se necessario.
 * @param stats Puntatore alla struttura delle statistiche.
 * @param filename Nome del file dove si trova l'errore.
 * @param line Numero di riga dell'errore.
 * @param identifier Nome dell'identificatore errato.
 */
void add_identifier_error(ProcessingStats* stats, const char* filename, int line, const char* identifier) {
    stats->errors_found++;
    if (stats->errors_found > stats->error_capacity) {
        int new_capacity = (stats->error_capacity == 0) ? 10 : stats->error_capacity * 2;
        IdentifierError* new_errors = realloc(stats->errors, new_capacity * sizeof(IdentifierError));
        if (!new_errors) {
            perror("Errore: Impossibile riallocare memoria per gli errori");
            stats->errors_found--;
            return;
        }
        stats->errors = new_errors;
        stats->error_capacity = new_capacity;
    }

    int current_error_index = stats->errors_found - 1;

    // Alloca e copia il nome del file
    size_t filename_len = strlen(filename);
    stats->errors[current_error_index].filename = malloc(filename_len + 1);
    if (stats->errors[current_error_index].filename) {
        strcpy(stats->errors[current_error_index].filename, filename);
    } else {
        perror("malloc fallito per filename in add_identifier_error");
        stats->errors[current_error_index].filename = NULL;
    }

    // Alloca e copia il nome dell'identificatore
    size_t identifier_len = strlen(identifier);
    stats->errors[current_error_index].identifier_name = malloc(identifier_len + 1);
    if (stats->errors[current_error_index].identifier_name) {
        strcpy(stats->errors[current_error_index].identifier_name, identifier);
    } else {
        perror("malloc fallito per identifier_name in add_identifier_error");
        stats->errors[current_error_index].identifier_name = NULL;
    }

    // Fallback per filename in caso di errore di allocazione
    if (!stats->errors[current_error_index].filename) {
        const char* error_str = "(alloc error)";
        size_t error_str_len = strlen(error_str);
        stats->errors[current_error_index].filename = malloc(error_str_len + 1);
        if (stats->errors[current_error_index].filename) {
            strcpy(stats->errors[current_error_index].filename, error_str);
        }
    }
    // Nessun fallback per identifier_name: già segnalato da perror
}

/**
 * Aggiunge le statistiche di un file incluso all'array dinamico.
 * Rialloca l'array se necessario.
 * @param stats Puntatore alla struttura delle statistiche.
 * @param filename Nome del file incluso.
 * @param size Dimensione del file in byte.
 * @param lines Numero di righe del file.
 */
void add_included_file_stats(ProcessingStats* stats, const char* filename, long size, int lines) {
    int index = stats->includes_processed - 1; // Indice dove inserire (0-based)

    if (stats->includes_processed > stats->included_files_capacity) {
        int new_capacity = (stats->included_files_capacity == 0) ? 5 : stats->included_files_capacity * 2;
        FileStats* new_included_stats = realloc(stats->included_files_stats, new_capacity * sizeof(FileStats));
        if (!new_included_stats) {
            perror("Errore: Impossibile riallocare memoria per le statistiche dei file inclusi");
            stats->includes_processed--;
            return;
        }
        stats->included_files_stats = new_included_stats;
        stats->included_files_capacity = new_capacity;
    }

    if (index < 0 || index >= stats->included_files_capacity) {
        fprintf(stderr, "Errore logico: indice file incluso non valido in add_included_file_stats.\n");
        return;
    }

    // Alloca e copia il nome del file incluso
    size_t filename_len = strlen(filename);
    stats->included_files_stats[index].filename = malloc(filename_len + 1);
    if (stats->included_files_stats[index].filename) {
        strcpy(stats->included_files_stats[index].filename, filename);
    } else {
        perror("malloc fallito per filename in add_included_file_stats");
        stats->included_files_stats[index].filename = NULL;
    }

    stats->included_files_stats[index].size_bytes = size;
    stats->included_files_stats[index].lines = lines;

    // Fallback per filename in caso di errore di allocazione
    if (!stats->included_files_stats[index].filename) {
        const char* error_str = "(alloc error)";
        size_t error_str_len = strlen(error_str);
        stats->included_files_stats[index].filename = malloc(error_str_len + 1);
        if (stats->included_files_stats[index].filename) {
            strcpy(stats->included_files_stats[index].filename, error_str);
        }
    }
}

/**
 * Stampa le statistiche di elaborazione su uno stream specificato (stdout o stderr).
 * Mostra informazioni su file di input, file inclusi, variabili, errori, commenti e output.
 * @param stats Puntatore alla struttura delle statistiche.
 * @param stream Stream di output (stdout o stderr).
 */
void print_stats(const ProcessingStats* stats, FILE* stream) {
    if (!stats->verbose) return;

    fprintf(stream, "\n--- Statistiche di Elaborazione ---\n");

    // File Input
    fprintf(stream, "File Input:\n");
    if (stats->input_file_stats.filename) {
        fprintf(stream, "  Nome: %s\n", stats->input_file_stats.filename);
        fprintf(stream, "  Dimensione (pre): %ld bytes\n", stats->input_file_stats.size_bytes);
        fprintf(stream, "  Righe (pre): %d\n", stats->input_file_stats.lines);
    } else {
        fprintf(stream, "  (Statistiche file input non disponibili)\n");
    }

    // Files Inclusi
    fprintf(stream, "File Inclusi (%d):\n", stats->includes_processed);
    for (int i = 0; i < stats->includes_processed; ++i) {
        const char* fname = (stats->included_files_stats && stats->included_files_stats[i].filename) ? stats->included_files_stats[i].filename : "(sconosciuto o errore allocazione)";
        fprintf(stream, "  - Nome: %s\n", fname);
        fprintf(stream, "    Dimensione (pre): %ld bytes\n", (stats->included_files_stats ? stats->included_files_stats[i].size_bytes : -1));
        fprintf(stream, "    Righe (pre): %d\n", (stats->included_files_stats ? stats->included_files_stats[i].lines : -1));
    }

    // Variabili ed Errori
    fprintf(stream, "Controllo Variabili:\n");
    fprintf(stream, "  Variabili controllate: %d\n", stats->vars_checked);
    fprintf(stream, "  Errori identificatore rilevati: %d\n", stats->errors_found);
    for (int i = 0; i < stats->errors_found; ++i) {
        const char* err_fname = (stats->errors && stats->errors[i].filename) ? stats->errors[i].filename : "(sconosciuto o errore allocazione)";
        const char* err_id = (stats->errors && stats->errors[i].identifier_name) ? stats->errors[i].identifier_name : "(sconosciuto o errore allocazione)";
        fprintf(stream, "  - Errore: Identificatore non valido '%s' nel file '%s' alla riga %d\n",
                err_id,
                err_fname,
                (stats->errors ? stats->errors[i].line_number : -1));
    }

    // Commenti
    fprintf(stream, "Commenti:\n");
    fprintf(stream, "  Righe di commento eliminate: %d\n", stats->comments_removed);

    // Output
    fprintf(stream, "Output:\n");
    fprintf(stream, "  Righe totali: %d\n", stats->output_lines);
    fprintf(stream, "  Dimensione totale: %ld bytes\n", stats->output_size_bytes);

    fprintf(stream, "--- Fine Statistiche ---\n");
}

/**
 * Libera tutta la memoria allocata dinamicamente nella struttura delle statistiche.
 * Dopo la chiamata, la struttura è riportata allo stato iniziale.
 * @param stats Puntatore alla struttura da liberare.
 */
void free_stats(ProcessingStats* stats) {
    if (!stats) return;

    // Libera filename del file di input
    free(stats->input_file_stats.filename);
    stats->input_file_stats.filename = NULL;

    // Libera memoria per ogni errore registrato
    for (int i = 0; i < stats->errors_found; ++i) {
        free(stats->errors[i].filename);
        free(stats->errors[i].identifier_name);
    }
    free(stats->errors);
    stats->errors = NULL;
    stats->errors_found = 0;
    stats->error_capacity = 0;

    // Libera memoria per ogni file incluso registrato
    for (int i = 0; i < stats->includes_processed; ++i) {
        if (stats->included_files_stats && stats->included_files_stats[i].filename) {
            free(stats->included_files_stats[i].filename);
        }
    }
    free(stats->included_files_stats);
    stats->included_files_stats = NULL;
    stats->includes_processed = 0;
    stats->included_files_capacity = 0;

    // Resetta i contatori semplici
    stats->vars_checked = 0;
    stats->comments_removed = 0;
    stats->output_lines = 0;
    stats->output_size_bytes = 0;
}

/**
 * Verifica se una stringa è un identificatore C valido.
 * Non controlla se è una parola chiave riservata.
 * @param str Stringa da verificare.
 * @return true se valido, false altrimenti.
 */
bool is_valid_c_identifier(const char* str) {
    if (!str || !(*str)) {
        return false;
    }
    // Il primo carattere deve essere una lettera o un underscore
    if (!isalpha((unsigned char)*str) && *str != '_') {
        return false;
    }
    // I caratteri successivi possono essere lettere, numeri o underscore
    str++;
    while (*str) {
        if (!isalnum((unsigned char)*str) && *str != '_') {
            return false;
        }
        str++;
    }
    return true;
}

/**
 * Estrae il nome del file da una direttiva #include "filename.h".
 * Restituisce una stringa allocata dinamicamente (da liberare) o NULL in caso di errore.
 * Supporta solo la sintassi con doppi apici.
 * @param line Riga contenente la direttiva #include.
 * @return Puntatore a stringa allocata o NULL.
 */
char* extract_include_filename(const char* line) {
    const char* start_quote = strchr(line, '"');
    if (!start_quote) {
        return NULL;
    }

    const char* end_quote = strchr(start_quote + 1, '"');
    if (!end_quote) {
        fprintf(stderr, "Attenzione: direttiva #include malformata (manca la seconda '\"').\n");
        return NULL;
    }

    size_t len = end_quote - (start_quote + 1);
    if (len == 0) {
        fprintf(stderr, "Attenzione: direttiva #include con nome file vuoto.\n");
        return NULL;
    }

    char* filename = malloc(len + 1);
    if (!filename) {
        perror("Errore: malloc fallito per nome file include");
        return NULL;
    }

    strncpy(filename, start_quote + 1, len);
    filename[len] = '\0';

    return filename;
}