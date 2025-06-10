#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include "myPreCompiler.h"

// Dimensione massima di una riga letta dal file sorgente
#define MAX_LINE_LEN 4096
// Profondità massima consentita per l'inclusione ricorsiva di file (#include)
#define MAX_INCLUDE_DEPTH 10

// Rimozione dei commenti dal codice sorgente
typedef enum {
    CODE,           // Analisi normale del codice (non in commento)
    SLASH,          // Trovato '/' in attesa di capire se è l'inizio di un commento
    BLOCK_COMMENT,  // All'interno di un commento multi-linea /* ... */
    LINE_COMMENT,   // All'interno di un commento su singola riga //
    STAR_IN_BLOCK   // Trovato '*' all'interno di un commento multi-linea
} CommentState;

// Parsing delle dichiarazioni di variabili
typedef enum {
    PRE_MAIN,                 // Prima di trovare la funzione 'main'
    GLOBAL_DECL,              // Analisi delle dichiarazioni globali (prima di 'main')
    IN_MAIN_FIND_OPEN_BRACE,  // Trovata la dichiarazione di 'main', in attesa della '{'
    IN_MAIN_LOCAL_DECL,       // All'interno di 'main', ricerca di dichiarazioni locali
    IN_MAIN_CODE,             // All'interno di 'main', analisi del codice vero e proprio
    POST_MAIN                 // Dopo la chiusura di 'main' (stato teorico, non dovrebbe verificarsi)
} ParsingState;

// =====================
// Funzioni di utilità
// =====================

/**
 * Calcola la dimensione in byte e il numero di righe di un file di testo.
 * @param filename Nome del file da analizzare.
 * @param size Puntatore dove salvare la dimensione in byte.
 * @param lines Puntatore dove salvare il numero di righe.
 * @return true se l'operazione ha successo, false in caso di errore (es. file non trovato).
 */
bool get_file_pre_stats(const char* filename, long* size, int* lines) {
    *size = 0;
    *lines = 0;

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        // L'errore viene gestito dal chiamante
        return false;
    }

    // Calcola la dimensione del file
    if (fseek(fp, 0, SEEK_END) == 0) {
        *size = ftell(fp);
        if (*size == -1) {
             perror("Errore ftell");
             *size = 0;
        }
        rewind(fp);
    } else {
        perror("Errore fseek");
    }

    // Conta il numero di righe
    int ch;
    bool in_last_line = false;
    while ((ch = fgetc(fp)) != EOF) {
        in_last_line = true;
        if (ch == '\n') {
            (*lines)++;
            in_last_line = false;
        }
    }
    // Conta l'ultima riga se non termina con '\n'
    if (in_last_line) {
        (*lines)++;
    }

    fclose(fp);
    return true;
}

/**
 * Analizza una riga per identificare dichiarazioni di variabili e validare i nomi.
 * Aggiorna lo stato di parsing e le statistiche.
 * @param line Riga da analizzare (modificata dalla funzione).
 * @param line_num Numero della riga corrente.
 * @param current_filename Nome del file corrente.
 * @param p_state Puntatore allo stato di parsing.
 * @param stats Puntatore alla struttura delle statistiche.
 */
void process_declaration_line(char* line, int line_num, const char* current_filename, ParsingState* p_state, ProcessingStats* stats) {
    char* trimmed_line = line;
    while (isspace((unsigned char)*trimmed_line)) trimmed_line++;
    if (*trimmed_line == '\0') return; // Ignora righe vuote

    // Gestione delle parentesi graffe per cambiare stato
    if (*trimmed_line == '{') {
        if (*p_state == IN_MAIN_FIND_OPEN_BRACE) {
            *p_state = IN_MAIN_LOCAL_DECL;
        }
        return;
    }
    if (*trimmed_line == '}') {
        if (*p_state == IN_MAIN_CODE || *p_state == IN_MAIN_LOCAL_DECL) {
            *p_state = POST_MAIN;
        }
        return;
    }

    // Rileva l'inizio della funzione main
    if (*p_state == PRE_MAIN || *p_state == GLOBAL_DECL) {
        if (strstr(line, "main") != NULL && strchr(line, '(') != NULL) {
            *p_state = IN_MAIN_FIND_OPEN_BRACE;
            return;
        }
        if (*p_state == PRE_MAIN) {
            *p_state = GLOBAL_DECL;
        }
    } else if (*p_state == IN_MAIN_FIND_OPEN_BRACE) {
        // Attende la '{' dopo la dichiarazione di main
        return;
    }

    // Analizza dichiarazioni globali o locali
    if (*p_state == GLOBAL_DECL || *p_state == IN_MAIN_LOCAL_DECL) {
        // Verifica se la riga termina con ';'
        char* end = line + strlen(line) - 1;
        while(end >= line && isspace((unsigned char)*end)) end--;
        bool ends_with_semicolon = (end >= line && *end == ';');

        if (ends_with_semicolon) {
            // Estrae potenziali identificatori dalla dichiarazione
            size_t line_len = strlen(line);
            char* line_copy = malloc(line_len + 1);
            if (!line_copy) {
                perror("malloc fallito in process_declaration_line");
                return;
            }
            strcpy(line_copy, line);

            // Tokenizza la riga per trovare i nomi delle variabili
            char* token = strtok(line_copy, " \t\n\v\f\r,;*()[]=");
            bool first_token = true; // Il primo token è il tipo

            while (token != NULL) {
                if (!first_token) {
                    // Pulisce il token da eventuali spazi
                    while (isspace((unsigned char)*token)) token++;
                    char *tok_end = token + strlen(token) - 1;
                    while(tok_end > token && isspace((unsigned char)*tok_end)) tok_end--;
                    *(tok_end + 1) = '\0';

                    if (strlen(token) > 0) {
                        stats->vars_checked++;
                        if (!is_valid_c_identifier(token)) {
                            add_identifier_error(stats, current_filename, line_num, token);
                        }
                    }
                }
                first_token = false;
                token = strtok(NULL, " \t\n\v\f\r,;*()[]=");
            }
            free(line_copy);
        } else if (*p_state == IN_MAIN_LOCAL_DECL && *trimmed_line != '\0') {
            // Se la riga non è una dichiarazione, si passa all'analisi del codice vero e proprio
            *p_state = IN_MAIN_CODE;
        }
    }
}

/**
 * Funzione principale ricorsiva per processare un file C.
 * Rimuove i commenti, gestisce le direttive #include, analizza le dichiarazioni di variabili
 * e aggiorna le statistiche di elaborazione.
 * @param input_filename Nome del file da processare.
 * @param out_stream Stream di output su cui scrivere il codice processato.
 * @param stats Puntatore alla struttura delle statistiche.
 * @param depth Livello di profondità di inclusione (per evitare ricorsione infinita).
 * @return 0 in caso di successo, -1 in caso di errore grave.
 */
int process_c_file(const char* input_filename, FILE* out_stream, ProcessingStats* stats, int depth) {

    if (depth > MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Errore: Profondità massima di inclusione (%d) superata per il file '%s'. Possibile inclusione ricorsiva infinita.\n", MAX_INCLUDE_DEPTH, input_filename);
        return -1;
    }

    // Ottiene statistiche pre-processamento (dimensione e righe)
    long file_size = 0;
    int file_lines = 0;
    bool stats_ok = get_file_pre_stats(input_filename, &file_size, &file_lines);

    // Apre il file di input
    FILE* in_file = fopen(input_filename, "r");
    if (!in_file) {
        fprintf(stderr, "Errore: Impossibile aprire il file di input '%s': %s\n", input_filename, strerror(errno));
        return -1;
    }

    // Aggiorna le statistiche del file principale o dei file inclusi
    if (stats_ok) {
        if (depth == 0) {
            free(stats->input_file_stats.filename);
            size_t filename_len = strlen(input_filename);
            stats->input_file_stats.filename = malloc(filename_len + 1);
            if (!stats->input_file_stats.filename) {
                perror("malloc fallito per nome file input in process_c_file");
                stats->input_file_stats.filename = NULL;
            } else {
                strcpy(stats->input_file_stats.filename, input_filename);
            }
            stats->input_file_stats.size_bytes = file_size;
            stats->input_file_stats.lines = file_lines;
        } else {
            add_included_file_stats(stats, input_filename, file_size, file_lines);
        }
    } else if (depth > 0) {
        add_included_file_stats(stats, input_filename, -1, -1);
        fprintf(stderr, "Attenzione: Impossibile ottenere statistiche pre-processamento per il file incluso '%s'.\n", input_filename);
    } else {
        fprintf(stderr, "Attenzione: Impossibile ottenere statistiche pre-processamento per il file input '%s'.\n", input_filename);
    }

    // Buffer per la riga letta e per la riga processata (senza commenti)
    char line[MAX_LINE_LEN];
    char processed_line[MAX_LINE_LEN * 2];
    int current_line_num = 0;
    CommentState comment_state = CODE;     // Stato iniziale per la rimozione commenti
    ParsingState parsing_state = PRE_MAIN; // Stato iniziale per il parsing delle dichiarazioni

    // Ciclo principale di lettura e analisi del file riga per riga
    while (fgets(line, sizeof(line), in_file) != NULL) {
        current_line_num++;
        int processed_len = 0; // Lunghezza della riga processata (senza commenti)
        bool line_had_comment_flag = false; // Indica se la riga originale conteneva un commento
        bool is_include_directive = false;

        // 1. Gestione direttiva #include (analizza e processa ricorsivamente i file inclusi)
        char* trimmed_line_start = line;
        while(isspace((unsigned char)*trimmed_line_start)) trimmed_line_start++;

        if (strncmp(trimmed_line_start, "#include", 8) == 0) {
            is_include_directive = true;
            char* included_filename = extract_include_filename(trimmed_line_start);
            if (included_filename) {
                stats->includes_processed++;
                int include_result = process_c_file(included_filename, out_stream, stats, depth + 1);
                free(included_filename);

                if (include_result != 0) {
                    fprintf(stderr, "...Errore originato durante l'inclusione richiesta in '%s' riga %d.\n", input_filename, current_line_num);
                    fclose(in_file);
                    return -1;
                }
                continue; // Passa alla prossima riga dopo aver gestito l'include
            } else {
                fprintf(stderr, "Attenzione: Formato #include non valido o errore in '%s' riga %d. Riga trattata come codice.\n", input_filename, current_line_num);
                is_include_directive = false;
            }
        }

        // 2. Rimozione dei commenti dalla riga corrente
        processed_len = 0;
        processed_line[0] = '\0';
        bool current_line_is_fully_commented = false;

        for (int i = 0; line[i] != '\0'; ++i) {
            switch (comment_state) {
                case CODE:
                    if (line[i] == '/') {
                        comment_state = SLASH;
                    } else {
                        processed_line[processed_len++] = line[i];
                    }
                    break;
                case SLASH:
                    if (line[i] == '/') {
                        comment_state = LINE_COMMENT;
                        line_had_comment_flag = true;
                    } else if (line[i] == '*') {
                        comment_state = BLOCK_COMMENT;
                        line_had_comment_flag = true;
                    } else {
                        processed_line[processed_len++] = '/';
                        processed_line[processed_len++] = line[i];
                        comment_state = CODE;
                    }
                    break;
                case LINE_COMMENT:
                    // Ignora tutto fino a fine riga
                    if (line[i] == '\n') {
                        processed_line[processed_len++] = '\n';
                        comment_state = CODE;
                    }
                    break;
                case BLOCK_COMMENT:
                    line_had_comment_flag = true;
                    if (line[i] == '*') {
                        comment_state = STAR_IN_BLOCK;
                    } else if (line[i] == '\n') {
                        processed_line[processed_len++] = '\n';
                    }
                    break;
                case STAR_IN_BLOCK:
                    line_had_comment_flag = true;
                    if (line[i] == '/') {
                        comment_state = CODE;
                    } else if (line[i] == '*') {
                        // Rimane in STAR_IN_BLOCK
                    } else {
                        comment_state = BLOCK_COMMENT;
                        if (line[i] == '\n') {
                            processed_line[processed_len++] = '\n';
                        }
                    }
                    break;
            }
        }
        processed_line[processed_len] = '\0';

        // Verifica se la riga processata è vuota o ci sono solo spazi
        char* check_empty = processed_line;
        while(isspace((unsigned char)*check_empty)) check_empty++;
        current_line_is_fully_commented = (*check_empty == '\0');

        // Aggiorna il contatore dei commenti rimossi
        if (line_had_comment_flag && (current_line_is_fully_commented || comment_state == BLOCK_COMMENT || comment_state == STAR_IN_BLOCK)) {
            stats->comments_removed++;
        }

        // 3. Analisi delle dichiarazioni di variabili sulla riga processata
        if (!current_line_is_fully_commented) {
            char line_copy_for_analysis[MAX_LINE_LEN * 2];
            strncpy(line_copy_for_analysis, processed_line, sizeof(line_copy_for_analysis) -1);
            line_copy_for_analysis[sizeof(line_copy_for_analysis) - 1] = '\0';
            process_declaration_line(line_copy_for_analysis, current_line_num, input_filename, &parsing_state, stats);
        }

        // 4. Scrittura della riga processata sull'output, se non è completamente commentata
        if (!current_line_is_fully_commented) {
            if (fputs(processed_line, out_stream) == EOF) {
                perror("Errore durante la scrittura sul file di output");
                fclose(in_file);
                return -1;
            }
            stats->output_lines++;
            stats->output_size_bytes += processed_len;
        }
    }

    // Gestione di eventuali errori di lettura
    if (ferror(in_file)) {
        fprintf(stderr, "Errore durante la lettura del file '%s': %s\n", input_filename, strerror(errno));
        fclose(in_file);
        return -1;
    }

    // Chiusura del file di input
    if (fclose(in_file) != 0) {
        fprintf(stderr, "Errore durante la chiusura del file '%s': %s\n", input_filename, strerror(errno));
    }

    // Avviso se il file termina con un commento multi-linea non chiuso
    if (depth == 0 && (comment_state == BLOCK_COMMENT || comment_state == STAR_IN_BLOCK)) {
        fprintf(stderr, "Attenzione: Commento multi-riga /* ... */ non chiuso alla fine del file '%s'.\n", input_filename);
    }

    return 0;
}