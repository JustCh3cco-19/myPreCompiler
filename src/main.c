#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "precompiler.h"

static void usage(FILE *stream, const char *prog) {
    fprintf(stream,
        "Usage: %s [options] <input.c>\n"
        "Options:\n"
        " -i, --in <file>   file di input (obbligatorio se non passato come argomento libero)\n"
        " -o, --out <file>  file di output (stdout se omesso)\n"
        " -v, --verbose     stampa statistiche su stderr\n"
        " -h, --help        mostra questo messaggio\n",
        prog
    );
}

int main(int argc, char *argv[]) {
    config_t cfg = parsing_arguments(argc, argv);
    if (!cfg.valid) {
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    precompiler_stats_t stats = {0};

    // 1) preprocessing: include + rimozione commenti
    char *processed = preprocessing_file(cfg.input_file, &stats);

    // 2) validazione degli identificatori
    validate_identifiers(processed, &stats, cfg.input_file);

    // 3) emissione dell'output
    FILE *out = stdout;
    if (cfg.output_file) {
        out = fopen(cfg.output_file, "w");
        if (!out)
            handle_error("Cannot open output file", cfg.output_file, true);
    }
    fputs(processed, out);
    if (cfg.output_file)
        fclose(out);

    // 4) statistiche verbose su stderr
    if (cfg.verbose)
        print_stats(&stats);

    free(processed);
    return EXIT_SUCCESS;
}
