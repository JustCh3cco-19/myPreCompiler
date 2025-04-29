// Rimosso #define _POSIX_C_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      // Per strncmp, strchr, strrchr, strtok, strlen, strcpy
#include <stdbool.h>
// Rimosso #include <sys/stat.h>
#include <errno.h>
#include <ctype.h>       // Per isspace, isalpha, isalnum
#include "myPreCompiler.h"

#define MAX_LINE_LEN 4096       // Dimensione massima buffer riga
#define MAX_INCLUDE_DEPTH 10    // Limite per ricorsione include

// Stato per la rimozione commenti
typedef enum {
    CODE,
    SLASH,          // Visto un '/'
    BLOCK_COMMENT,  // Dentro /* ... */
    LINE_COMMENT,   // Dentro // ...
    STAR_IN_BLOCK   // Visto '*' dentro /* ... */
} CommentState;

// Stato per l'identificazione delle dichiarazioni (basato sulle assunzioni)
typedef enum {
    PRE_MAIN,                 // Prima di incontrare 'main'
    GLOBAL_DECL,              // In blocco dichiarazioni globali (prima di main)
    IN_MAIN_FIND_OPEN_BRACE,  // Trovato 'main', cercando '{'
    IN_MAIN_LOCAL_DECL,       // Dentro main, dopo '{', cercando dichiarazioni locali
    IN_MAIN_CODE,             // Dentro main, dopo le dichiarazioni locali
    POST_MAIN                 // Dopo la '}' di main (non dovrebbe accadere con le assunzioni)
} ParsingState;


// --- Funzioni Helper Locali ---

// Funzione per ottenere statistiche pre-processamento di un file
// Restituisce true se successo, false altrimenti (es. file non trovato)
bool get_file_pre_stats(const char* filename, long* size, int* lines) {
    *size = 0; // Inizializza
    *lines = 0;

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        // Non stampare errore qui, verrà gestito dal chiamante (process_c_file)
        return false;
    }

    // Dimensione usando fseek/ftell (standard C)
    if (fseek(fp, 0, SEEK_END) == 0) {
        *size = ftell(fp);
        if (*size == -1) {
             perror("Errore ftell");
             *size = 0; // Reset in caso di errore
        }
        rewind(fp); // Torna all'inizio per contare le righe
    } else {
        perror("Errore fseek");
    }


    // Conta Righe (standard C)
    int ch;
    bool in_last_line = false; // Per contare l'ultima riga se non termina con \n
    while ((ch = fgetc(fp)) != EOF) {
        in_last_line = true;
        if (ch == '\n') {
            (*lines)++;
            in_last_line = false; // La riga è terminata correttamente
        }
    }
    // Se abbiamo letto qualcosa e l'ultimo carattere non era \n, conta l'ultima riga
    if (in_last_line) {
        (*lines)++;
    }

    fclose(fp);
    return true;
}

// Processa una singola riga per identificare dichiarazioni e validare nomi
// Modifica lo stato di parsing e aggiorna le statistiche
void process_declaration_line(char* line, int line_num, const char* current_filename, ParsingState* p_state, ProcessingStats* stats) {

    char* trimmed_line = line;
    while (isspace((unsigned char)*trimmed_line)) trimmed_line++;
    if (*trimmed_line == '\0') return; // Ignora righe vuote o solo whitespace

    // Ignora le parentesi graffe stesse, ma usale per cambiare stato
    if (*trimmed_line == '{') {
        if (*p_state == IN_MAIN_FIND_OPEN_BRACE) {
            *p_state = IN_MAIN_LOCAL_DECL; // Inizia a cercare dichiarazioni locali
        }
        return;
    }
     if (*trimmed_line == '}') {
        if (*p_state == IN_MAIN_CODE || *p_state == IN_MAIN_LOCAL_DECL) {
            *p_state = POST_MAIN; // Fine main (teorica)
        }
        return;
     }


    // Cerca "main(" per cambiare stato (controllo molto semplice)
    if (*p_state == PRE_MAIN || *p_state == GLOBAL_DECL) {
        // Verifica se contiene "main" E una parentesi tonda aperta
        if (strstr(line, "main") != NULL && strchr(line, '(') != NULL) {
             // Assumiamo sia l'inizio della funzione main
            *p_state = IN_MAIN_FIND_OPEN_BRACE;
            return; // Passa alla prossima riga per cercare '{'
        }
         // Se siamo prima di main e non è una riga vuota/commento/include, assumiamo sia una dichiarazione globale
         // (Questa transizione avviene implicitamente se non si trova "main")
        if (*p_state == PRE_MAIN) {
             *p_state = GLOBAL_DECL;
        }

    } else if (*p_state == IN_MAIN_FIND_OPEN_BRACE) {
        // Siamo ancora alla ricerca della '{' dopo 'main(...)'
        // Abbiamo già gestito la '{' all'inizio della funzione.
        // Se arriviamo qui, significa che la riga non conteneva '{'.
        return; // Continua a cercare '{' sulla prossima riga
    }

    // Se siamo in stato di dichiarazione (globale o locale)
    if (*p_state == GLOBAL_DECL || *p_state == IN_MAIN_LOCAL_DECL) {
        // Controlla se la riga sembra una dichiarazione (euristica: finisce con ';')
        char* end = line + strlen(line) - 1;
        while(end >= line && isspace((unsigned char)*end)) end--; // Rimuovi spazi finali
        bool ends_with_semicolon = (end >= line && *end == ';');

        if (ends_with_semicolon) {
             // OK, tentativo di estrarre identificatori
             // Questa parte è MOLTO semplificata e soggetta a errori con sintassi complesse

             // Sostituzione di strdup con malloc + strcpy (standard C)
             size_t line_len = strlen(line);
             char* line_copy = malloc(line_len + 1);
             if (!line_copy) {
                 perror("malloc fallito in process_declaration_line");
                 // Gestione errore: non possiamo analizzare la riga, ma possiamo continuare
                 return; // Salta l'analisi di questa riga ma non terminare fatalmente
             }
             strcpy(line_copy, line);
             // Fine sostituzione


             // Tokenizza la riga (strtok è standard C)
             char* token = strtok(line_copy, " \t\n\v\f\r,;*()[]=");
             bool first_token = true; // Il primo token è il tipo (lo ignoriamo)

             while (token != NULL) {
                if (!first_token) {
                    // Rimuovi eventuali spazi bianchi iniziali/finali dal token residui
                    while (isspace((unsigned char)*token)) token++;
                    char *tok_end = token + strlen(token) - 1;
                    while(tok_end > token && isspace((unsigned char)*tok_end)) tok_end--;
                    *(tok_end + 1) = '\0'; // Termina la stringa

                    if (strlen(token) > 0) { // Abbiamo un potenziale identificatore
                        stats->vars_checked++;
                        if (!is_valid_c_identifier(token)) {
                            add_identifier_error(stats, current_filename, line_num, token);
                        }
                    }
                }
                first_token = false; // Dopo il primo, tutti sono potenziali identificatori
                token = strtok(NULL, " \t\n\v\f\r,;*()[]=");
             }
             free(line_copy); // Libera la memoria allocata
        } else if (*p_state == IN_MAIN_LOCAL_DECL && *trimmed_line != '\0') {
            // Se siamo DENTRO main (stato IN_MAIN_LOCAL_DECL), e troviamo una riga
            // che NON finisce con ';' e NON è vuota (o solo graffe),
            // assumiamo che le dichiarazioni locali contigue siano finite.
             *p_state = IN_MAIN_CODE;
        }
        // Se eravamo in GLOBAL_DECL e non finisce con ';', potrebbe essere
        // l'inizio di 'main' (già gestito sopra) o codice/dati globali
        // che non sono dichiarazioni di variabili semplici (es. #define, struct def).
        // Secondo le assunzioni, dovremmo avere solo dichiarazioni contigue prima di main.
    }
}


// Funzione principale ricorsiva per processare un file C
// Restituisce 0 in caso di successo, -1 in caso di errore grave
int process_c_file(const char* input_filename, FILE* out_stream, ProcessingStats* stats, int depth) {

    if (depth > MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Errore: Profondità massima di inclusione (%d) superata per il file '%s'. Possibile inclusione ricorsiva infinita.\n", MAX_INCLUDE_DEPTH, input_filename);
        return -1;
    }

    // --- Ottieni statistiche pre-processamento (usando standard C) ---
    long file_size = 0;
    int file_lines = 0;
    bool stats_ok = get_file_pre_stats(input_filename, &file_size, &file_lines);

    // Apri il file di input DOPO aver tentato di ottenere le stats
    FILE* in_file = fopen(input_filename, "r");
    if (!in_file) {
        fprintf(stderr, "Errore: Impossibile aprire il file di input '%s': %s\n", input_filename, strerror(errno));
        return -1; // Errore fatale se il file non si apre
    }

    // Registra le statistiche pre-processamento SE ottenute con successo E il file è stato aperto
    if (stats_ok) {
        if (depth == 0) { // File principale
             // Libera il precedente se per caso fosse stato allocato (improbabile)
             free(stats->input_file_stats.filename);

             // Sostituzione di strdup con malloc + strcpy (standard C)
             size_t filename_len = strlen(input_filename);
             stats->input_file_stats.filename = malloc(filename_len + 1);
             if (!stats->input_file_stats.filename) {
                 perror("malloc fallito per nome file input in process_c_file");
                 // Gestisci l'errore, ad esempio impostando il nome a NULL o a una stringa di errore
                 stats->input_file_stats.filename = NULL; // Imposta a NULL in caso di errore
             } else {
                 strcpy(stats->input_file_stats.filename, input_filename);
             }
             // Fine sostituzione

             stats->input_file_stats.size_bytes = file_size;
             stats->input_file_stats.lines = file_lines;
        } else { // File incluso (stats->includes_processed è stato già incrementato dal chiamante)
             add_included_file_stats(stats, input_filename, file_size, file_lines);
        }
    } else if (depth > 0) {
         // Se le stats non sono state ottenute ma il file è incluso, aggiungi con valori placeholder
         add_included_file_stats(stats, input_filename, -1, -1); // Usa -1 per indicare errore
         fprintf(stderr, "Attenzione: Impossibile ottenere statistiche pre-processamento per il file incluso '%s'.\n", input_filename);
    } else {
         fprintf(stderr, "Attenzione: Impossibile ottenere statistiche pre-processamento per il file input '%s'.\n", input_filename);
    }


    char line[MAX_LINE_LEN];                 // Buffer per la riga letta
    char processed_line[MAX_LINE_LEN * 2]; // Buffer per la riga processata (potrebbe allungarsi)
    int current_line_num = 0;
    CommentState comment_state = CODE;       // Stato iniziale rimozione commenti
    ParsingState parsing_state = PRE_MAIN;   // Stato iniziale parsing dichiarazioni

    while (fgets(line, sizeof(line), in_file) != NULL) {
        current_line_num++;
        int processed_len = 0; // Lunghezza della riga processata (senza commenti)
        bool line_had_comment_flag = false; // Flag se la riga originale conteneva un commento
        bool is_include_directive = false;

        // --- 1. Controllo Direttiva #include ---
        char* trimmed_line_start = line;
        while(isspace((unsigned char)*trimmed_line_start)) trimmed_line_start++; // Salta spazi iniziali

        if (strncmp(trimmed_line_start, "#include", 8) == 0) {
            is_include_directive = true; // Assumiamo sia un include per ora
            char* included_filename = extract_include_filename(trimmed_line_start);
            if (included_filename) {
                stats->includes_processed++; // Incrementa qui, PRIMA della chiamata ricorsiva
                // --- Chiamata Ricorsiva ---
                int include_result = process_c_file(included_filename, out_stream, stats, depth + 1);
                free(included_filename); // Libera il nome file allocato da extract...

                if (include_result != 0) {
                    // Errore durante l'elaborazione del file incluso
                    // L'errore specifico è già stato stampato dalla chiamata ricorsiva
                    fprintf(stderr, "...Errore originato durante l'inclusione richiesta in '%s' riga %d.\n", input_filename, current_line_num);
                    fclose(in_file);
                    return -1; // Propaga l'errore grave
                }
                // Se l'inclusione ha successo, salta il resto dell'elaborazione
                // per QUESTA riga (#include) e passa alla successiva.
                continue;
            } else {
                // Formato #include non riconosciuto o errore estrazione nome file
                // Stampa un warning ma considera la riga come codice normale
                fprintf(stderr, "Attenzione: Formato #include non valido o errore in '%s' riga %d. Riga trattata come codice.\n", input_filename, current_line_num);
                is_include_directive = false; // Non era un include valido, processala normalmente
            }
        }

        // --- 2. Rimozione Commenti (se non era un include gestito sopra) ---
        //    Scrive i caratteri non-commento in processed_line
        processed_len = 0;
        processed_line[0] = '\0';
        bool current_line_is_fully_commented = false; // Flag se la riga diventa vuota dopo rimozione commenti

        for (int i = 0; line[i] != '\0'; ++i) {
            switch (comment_state) {
                case CODE:
                    if (line[i] == '/') {
                        comment_state = SLASH;
                        // Non scrivere ancora la slash
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
                        // Era solo una slash normale
                        processed_line[processed_len++] = '/'; // Scrivi la slash precedente
                        processed_line[processed_len++] = line[i]; // Scrivi carattere corrente
                        comment_state = CODE;
                    }
                    break;
                case LINE_COMMENT:
                    // Ignora tutto fino a fine riga
                    if (line[i] == '\n') {
                        processed_line[processed_len++] = '\n'; // Mantieni il newline
                        comment_state = CODE; // Il commento finisce qui
                    }
                    // Altrimenti ignora il carattere
                    break;
                case BLOCK_COMMENT:
                    line_had_comment_flag = true; // Siamo dentro un commento blocco
                    if (line[i] == '*') {
                        comment_state = STAR_IN_BLOCK;
                    } else if (line[i] == '\n') {
                         processed_line[processed_len++] = '\n'; // Mantieni newline ma resta in commento blocco
                    }
                    // Altrimenti ignora il carattere (fa parte del commento blocco)
                    break;
                case STAR_IN_BLOCK:
                    line_had_comment_flag = true;
                    if (line[i] == '/') {
                        comment_state = CODE; // Fine commento blocco
                    } else if (line[i] == '*') {
                        // Sequenza '**', resta in STAR_IN_BLOCK
                    } else {
                        // Era solo una stella isolata nel commento blocco
                        comment_state = BLOCK_COMMENT;
                        if (line[i] == '\n') { // Se la stella era seguita da newline
                             processed_line[processed_len++] = '\n';
                        }
                        // Altrimenti ignora il carattere (era tipo '*a')
                    }
                    break;
            } // End switch
        } // End for loop caratteri riga
        processed_line[processed_len] = '\0'; // Termina la stringa processata

        // Controlla se la riga processata è diventata vuota o solo whitespace
        char* check_empty = processed_line;
        while(isspace((unsigned char)*check_empty)) check_empty++;
        current_line_is_fully_commented = (*check_empty == '\0');

        // Aggiorna contatore commenti SE la riga originale conteneva un commento
        // E se la riga processata è vuota/whitespace O se siamo ancora dentro un blocco commento
        if (line_had_comment_flag && (current_line_is_fully_commented || comment_state == BLOCK_COMMENT || comment_state == STAR_IN_BLOCK)) {
            stats->comments_removed++;
        }

        // --- 3. Controllo Dichiarazioni Variabili (sulla riga *processata* e *non vuota*) ---
        if (!current_line_is_fully_commented) {
            // Copia la linea processata per l'analisi (strtok modifica)
            // Usa un buffer temporaneo sullo stack se possibile, o alloca dinamicamente
            char line_copy_for_analysis[MAX_LINE_LEN * 2];
            strncpy(line_copy_for_analysis, processed_line, sizeof(line_copy_for_analysis) -1);
            line_copy_for_analysis[sizeof(line_copy_for_analysis) - 1] = '\0'; // Ensure null termination

            process_declaration_line(line_copy_for_analysis, current_line_num, input_filename, &parsing_state, stats);
        }

        // --- 4. Scrittura Output (della riga processata, se non è completamente commentata) ---
        if (!current_line_is_fully_commented) {
            if (fputs(processed_line, out_stream) == EOF) {
                perror("Errore durante la scrittura sul file di output");
                fclose(in_file);
                return -1; // Errore fatale di scrittura
            }
            stats->output_lines++;
            // Aggiungi la lunghezza della stringa scritta (escluso il terminatore nullo)
            // Nota: strlen non include il newline se fputs lo scrive, ma è una buona approssimazione
            stats->output_size_bytes += processed_len;
        }

    } // End while fgets

    // Controlla errori di lettura dopo il loop
    if (ferror(in_file)) {
        fprintf(stderr, "Errore durante la lettura del file '%s': %s\n", input_filename, strerror(errno));
        fclose(in_file); // Tenta comunque la chiusura
        return -1; // Errore di lettura
    }

    // Chiusura file input
    if (fclose(in_file) != 0) {
        fprintf(stderr, "Errore durante la chiusura del file '%s': %s\n", input_filename, strerror(errno));
        // Non considerato errore fatale per il return code, ma è un problema
    }

     // Warning se si esce dal file principale ancora dentro un commento blocco
     if (depth == 0 && (comment_state == BLOCK_COMMENT || comment_state == STAR_IN_BLOCK)) {
        fprintf(stderr, "Attenzione: Commento multi-riga /* ... */ non chiuso alla fine del file '%s'.\n", input_filename);
     }


    return 0; // Successo per questo file
}