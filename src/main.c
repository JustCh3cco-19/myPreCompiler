#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "myPreCompiler.h"

/**
 * Stampa le istruzioni d'uso del programma su stderr.
 * @param prog_name Nome dell'eseguibile (tipicamente argv[0]).
 */
void print_usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s [-v] [-o <output_file>] -i <input_file.c>\n", prog_name);
    fprintf(stderr, "   o: %s [-v] [-o <output_file>] <input_file.c>\n", prog_name);
    fprintf(stderr, "\nOpzioni:\n");
    fprintf(stderr, "  -i <file>          Specifica il file C di input (obbligatorio, può essere primo argomento).\n");
    fprintf(stderr, "  -o <file>          Specifica il file di output. Se omesso, usa stdout.\n");
    fprintf(stderr, "  -v                 Abilita l'output delle statistiche di elaborazione (su stderr).\n");
    fprintf(stderr, "  <input_file.c>     Alternativa per specificare l'input se è il primo argomento.\n");
}

/**
 * Funzione principale del precompilatore.
 * Gestisce il parsing degli argomenti, apre i file di input/output,
 * chiama il pre-processing e stampa statistiche e messaggi di stato.
 */
int main(int argc, char *argv[]) {
    char* input_filename = NULL;     // Nome del file di input C da processare
    char* output_filename = NULL;    // Nome del file di output (opzionale)
    bool verbose_mode = false;       // Flag per abilitare la stampa delle statistiche

    // --- Parsing manuale degli argomenti della riga di comando ---
    // Supporta: -i <input>, -o <output>, -v, oppure input come primo argomento
    int i;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') { // Opzione con '-'
            if (strcmp(argv[i], "-i") == 0) {
                if (input_filename != NULL) {
                    fprintf(stderr, "Errore: Opzione -i specificata più volte.\n");
                    print_usage(argv[0]);
                    return 1;
                }
                if (i + 1 < argc) {
                    input_filename = argv[++i]; // Prendi il file di input
                } else {
                    fprintf(stderr, "Errore: L'opzione -i richiede un argomento (nome file).\n");
                    print_usage(argv[0]);
                    return 1;
                }
            } else if (strcmp(argv[i], "-o") == 0) {
                if (output_filename != NULL) {
                    fprintf(stderr, "Errore: Opzione -o specificata più volte.\n");
                    print_usage(argv[0]);
                    return 1;
                }
                if (i + 1 < argc) {
                    output_filename = argv[++i]; // Prendi il file di output
                } else {
                    fprintf(stderr, "Errore: L'opzione -o richiede un argomento (nome file).\n");
                    print_usage(argv[0]);
                    return 1;
                }
            } else if (strcmp(argv[i], "-v") == 0) {
                verbose_mode = true; // Abilita modalità verbosa
            } else {
                fprintf(stderr, "Errore: Opzione non riconosciuta '%s'.\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else { // Argomento senza '-': considerato come file di input se non già specificato
            if (input_filename == NULL) {
                input_filename = argv[i];
            } else {
                fprintf(stderr, "Errore: Argomento non riconosciuto '%s' dopo il file di input.\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        }
    }

    // Verifica che il file di input sia stato specificato
    if (input_filename == NULL) {
        fprintf(stderr, "Errore: File di input non specificato (usare -i <file> o specificarlo come primo argomento).\n");
        print_usage(argv[0]);
        return 1;
    }
    // --- Fine parsing argomenti ---

    // Determina lo stream di output: stdout di default, oppure file se richiesto
    FILE* out_stream = stdout;
    bool custom_output_file = false; // Serve per sapere se chiudere out_stream
    if (output_filename != NULL) {
        out_stream = fopen(output_filename, "w");
        if (!out_stream) {
            fprintf(stderr, "Errore: Impossibile aprire il file di output '%s': %s\n", output_filename, strerror(errno));
            return 1;
        }
        custom_output_file = true;
    }

    // Inizializza la struttura delle statistiche di elaborazione
    ProcessingStats stats;
    init_stats(&stats, verbose_mode);

    // --- Avvia il pre-processing del file ---
    fprintf(stderr, "Processando il file: %s\n", input_filename);
    int result = process_c_file(input_filename, out_stream, &stats, 0); // 0 = profondità iniziale

    // --- Operazioni di chiusura e stampa risultati ---
    // Chiudi il file di output solo se non è stdout
    if (custom_output_file) {
        if (fclose(out_stream) != 0) {
            fprintf(stderr, "Errore durante la chiusura del file di output '%s': %s\n", output_filename, strerror(errno));
            if (result == 0) result = 1;
        } else if (result == 0) {
            fprintf(stderr, "File processato scritto con successo in: %s\n", output_filename);
        }
    } else if (result == 0) {
        fprintf(stderr, "File processato scritto su stdout.\n");
    }

    // Stampa statistiche di elaborazione se richiesto
    if (verbose_mode) {
        print_stats(&stats, stderr);
    }

    // Libera memoria allocata per le statistiche
    free_stats(&stats);

    // Messaggio finale di stato
    if (result == 0) {
        fprintf(stderr, "Elaborazione completata con successo.\n");
    } else {
        fprintf(stderr, "Elaborazione terminata con errori.\n");
    }

    return (result == 0) ? 0 : 1; // 0 = successo, 1 = errore
}