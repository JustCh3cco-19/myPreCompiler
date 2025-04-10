#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "precompiler.h"

// funzione per leggere il contenuto di un file
char *read_file_content(const char *filename, long *size, int *lines) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        handle_error("Errore nell'apertura del file incluso", filename, true);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    char *content = malloc(file_size + 1);
    if (!content) {
        fclose(f);
        handle_error("Errore di allocazione durante la lettura dell'include", filename, true);
        return NULL;
    }

    size_t read_bytes = fread(content, 1, file_size, f);
    content[read_bytes] = '\0';

    fclose(f);

    // Conta le righe
    int line_count = 0;
    for (size_t i = 0; i < read_bytes; ++i) {
        if (content[i] == '\n') line_count++;
    }

    if (size) *size = read_bytes;
    if (lines) *lines = line_count;

    return content;
}

// funzione per risolvere i commenti
char *resolve_includes(const char *code, precompiler_stats_t *stats) {
    // Si stima una dimensione iniziale, da modificare se necessario
    size_t initial_size = strlen(code) * 2 + 1;
    char *output = malloc(initial_size);
    if (!output) {
        handle_error("Allocazione memoria fallita in resolve_includes", NULL, true);
        return NULL;
    }
    // Puntatore al termine attuale dell'output
    char *tail = output;
    *tail = '\0';

    char line[1024];
    const char *ptr = code;

    while (*ptr) {
        int i = 0;
        while (*ptr && *ptr != '\n' && i < 1023) {
            line[i++] = *ptr++;
        }
        if (*ptr == '\n') ptr++;
        line[i] = '\0';

        if (strncmp(line, "#include", 8) == 0) {
            const char *start = strchr(line, '"');
            const char *end = strrchr(line, '"');
            if (!start || !end || start == end) {
                start = strchr(line, '<');
                end = strchr(line, '>');
            }
            if (start && end && end > start) {
                char filename[256];
                strncpy(filename, start + 1, end - start - 1);
                filename[end - start - 1] = '\0';

                long inc_size;
                int inc_lines;
                char *included_code = read_file_content(filename, &inc_size, &inc_lines);
                if (included_code) {
                    char *resolved = resolve_includes(included_code, stats);
                    
                    // Occupa spazio per la nuova stringa: eventualmente controlla e rialloca se necessario.
                    size_t len_resolved = strlen(resolved);
                    size_t curr_size = tail - output;
                    if (curr_size + len_resolved + 1 > initial_size) {
                        // Rialloca il buffer per ospitare il nuovo contenuto
                        initial_size = (curr_size + len_resolved + 1) * 2;
                        output = realloc(output, initial_size);
                        if (!output) {
                            handle_error("Riallocazione memoria fallita in resolve_includes", NULL, true);
                            free(resolved);
                            free(included_code);
                            return NULL;
                        }
                        tail = output + curr_size;
                    }
                    // Copia il contenuto risolto direttamente nel buffer
                    memcpy(tail, resolved, len_resolved);
                    tail += len_resolved;
                    *tail = '\0';

                    free(resolved);
                    free(included_code);
                    stats->num_files_included++;
                    stats->input_size += inc_size;
                    stats->input_lines += inc_lines;
                }
                continue;
            }
        }

        // Append della linea corrente e dell'andata a capo
        size_t len_line = strlen(line);
        size_t curr_size = tail - output;
        if (curr_size + len_line + 2 > initial_size) { // +2 per \n e \0
            initial_size = (curr_size + len_line + 2) * 2;
            output = realloc(output, initial_size);
            if (!output) {
                handle_error("Riallocazione memoria fallita in resolve_includes", NULL, true);
                return NULL;
            }
            tail = output + curr_size;
        }
        memcpy(tail, line, len_line);
        tail += len_line;
        *tail++ = '\n';
        *tail = '\0';
    }
    return output;
}


// funzione per stampare le statistiche
void print_stats(const precompiler_stats_t *stats) { 
    fprintf(stderr, "Variabili controllate: %d\n", stats->num_variables);
    fprintf(stderr, "Errori rilevati: %d\n", stats->num_errors);
    fprintf(stderr, "Commenti eliminati: %d\n", stats->num_comments_removed);
    fprintf(stderr, "File inclusi: %d\n", stats->num_files_included);
    fprintf(stderr, "Righe input: %d - Byte: %ld\n", stats->input_lines, stats->input_size);
    fprintf(stderr, "Righe output: %d - Byte: %ld\n", stats->output_lines, stats->output_size);
}