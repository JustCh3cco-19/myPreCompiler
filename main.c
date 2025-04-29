#include <stdio.h>
#include <stdlib.h>
#include <string.h>      // Per strcmp, strncmp (per parsing manuale)
#include <stdbool.h>
#include <errno.h>
// Rimosso #include <unistd.h> (per getopt)
#include "myPreCompiler.h"

// Funzione per stampare le istruzioni d'uso
void print_usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s [-v] [-o <output_file>] -i <input_file.c>\n", prog_name);
    fprintf(stderr, "   o: %s [-v] [-o <output_file>] <input_file.c>\n", prog_name);
    fprintf(stderr, "\nOpzioni:\n");
    fprintf(stderr, "  -i <file>          Specifica il file C di input (obbligatorio, può essere primo argomento).\n");
    fprintf(stderr, "  -o <file>          Specifica il file di output. Se omesso, usa stdout.\n");
    fprintf(stderr, "  -v                 Abilita l'output delle statistiche di elaborazione (su stderr).\n");
    fprintf(stderr, "  <input_file.c>     Alternativa per specificare l'input se è il primo argomento.\n");
}

int main(int argc, char *argv[]) {
    char* input_filename = NULL;
    char* output_filename = NULL;
    bool verbose_mode = false;

    // --- Parsing manuale degli argomenti ---
    // Rimosso getopt, optarg, optind
    int i;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') { // È un'opzione
            if (strcmp(argv[i], "-i") == 0) {
                if (input_filename != NULL) {
                    fprintf(stderr, "Errore: Opzione -i specificata più volte.\n");
                    print_usage(argv[0]);
                    return 1;
                }
                if (i + 1 < argc) {
                    input_filename = argv[++i]; // Prendi l'argomento successivo e salta un'altra iterazione
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
                    output_filename = argv[++i]; // Prendi l'argomento successivo e salta un'altra iterazione
                } else {
                    fprintf(stderr, "Errore: L'opzione -o richiede un argomento (nome file).\n");
                    print_usage(argv[0]);
                    return 1;
                }
            } else if (strcmp(argv[i], "-v") == 0) {
                verbose_mode = true;
            } else {
                fprintf(stderr, "Errore: Opzione non riconosciuta '%s'.\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else { // Non è un'opzione
            if (input_filename == NULL) {
                // Questo è il primo argomento non-opzione, lo consideriamo il file di input
                input_filename = argv[i];
            } else {
                fprintf(stderr, "Errore: Argomento non riconosciuto '%s' dopo il file di input.\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        }
    }

    // Controllo obbligatorio: input filename deve essere stato specificato in un modo o nell'altro
    if (input_filename == NULL) {
        fprintf(stderr, "Errore: File di input non specificato (usare -i <file> o specificarlo come primo argomento).\n");
        print_usage(argv[0]);
        return 1;
    }
    // --- Fine Parsing manuale ---


    // Determina lo stream di output
    FILE* out_stream = stdout;
    bool custom_output_file = false; // Flag per sapere se dobbiamo chiudere out_stream
    if (output_filename != NULL) {
        out_stream = fopen(output_filename, "w");
        if (!out_stream) {
            fprintf(stderr, "Errore: Impossibile aprire il file di output '%s': %s\n", output_filename, strerror(errno));
            // Non c'è bisogno di liberare stats qui perché non sono ancora state popolate
            return 1; // Errore fatale
        }
        custom_output_file = true;
    }

    // Inizializza la struttura delle statistiche
    ProcessingStats stats;
    init_stats(&stats, verbose_mode);

    // --- Esegui il Processamento ---
    fprintf(stderr, "Processando il file: %s\n", input_filename); // Feedback utente
    int result = process_c_file(input_filename, out_stream, &stats, 0); // 0 è la profondità iniziale

    // --- Operazioni Post-Processamento ---

    // Chiudi il file di output SOLO se ne è stato aperto uno specifico (non stdout)
    if (custom_output_file) {
        if (fclose(out_stream) != 0) {
            fprintf(stderr, "Errore durante la chiusura del file di output '%s': %s\n", output_filename, strerror(errno));
            // Se la processazione era andata bene, ora segnala errore
            if (result == 0) result = 1;
        } else if (result == 0) {
             fprintf(stderr, "File processato scritto con successo in: %s\n", output_filename);
        }
    } else if (result == 0) {
         fprintf(stderr, "File processato scritto su stdout.\n");
    }


    // Stampa Statistiche (se richieste) su stderr
    // Vengono stampate anche se result != 0 per mostrare cosa è stato fatto fino all'errore
    if (verbose_mode) {
        print_stats(&stats, stderr);
    }

    // Libera memoria allocata per le statistiche
    free_stats(&stats);

    // Stampa messaggio finale di successo/errore
    if (result == 0) {
        fprintf(stderr, "Elaborazione completata con successo.\n");
    } else {
        fprintf(stderr, "Elaborazione terminata con errori.\n");
    }


    return (result == 0) ? 0 : 1; // Ritorna 0 per successo, 1 per errore
}