#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include "precompiler.h"

/*=====================================================================
 *  Usage e parsing delle opzioni
 *===================================================================*/
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

config_t parsing_arguments(int argc, char *argv[])
{
    config_t cfg = { 
        .input_file   = NULL, 
        .output_file  = NULL,
        .verbose      = false, 
        .valid        = false 
    };
    bool error = false;
    int  i = 1;

    while (i < argc && !error) {
        const char *arg = argv[i];
        if (arg[0] != '-') {
            if (!cfg.input_file)
                cfg.input_file = arg;
            else
                handle_error("Extra positional argument", arg, !(error = true));
            ++i;
            continue;
        }
        if (strcmp(arg, "--") == 0) {
            ++i;
            if (i < argc && !cfg.input_file)
                cfg.input_file = argv[i++];
            else if (i < argc) {
                handle_error("Extra positional argument", argv[i], !(error = true));
                ++i;
            }
            break;
        }
        if (strncmp(arg, "--", 2) == 0) {
            const char *opt = arg + 2;
            const char *val = NULL;
            const char *eq  = strchr(opt, '=');
            if (eq) val = eq + 1;
            switch (opt[0]) {
            case 'i':
                if (strncmp(opt, "in", 2) == 0) {
                    if (!val) { if (++i >= argc) { handle_error("Missing arg for --in",NULL,!(error=true)); break; } val = argv[i]; }
                    if (cfg.input_file) handle_error("Input dup", val, !(error=true));
                    else cfg.input_file = val;
                    ++i; continue;
                }
                break;
            case 'o':
                if (strncmp(opt, "out", 3) == 0) {
                    if (!val) { if (++i >= argc) { handle_error("Missing arg for --out",NULL,!(error=true)); break; } val = argv[i]; }
                    if (cfg.output_file) handle_error("Output dup", val, !(error=true));
                    else cfg.output_file = val;
                    ++i; continue;
                }
                break;
            case 'v':
                if (strcmp(opt, "verbose") == 0) {
                    if (val) handle_error("--verbose no value", val, !(error=true));
                    cfg.verbose = true;
                    ++i; continue;
                }
                break;
            }
            handle_error("Unknown long option", arg, !(error=true));
            continue;
        }
        switch (arg[1]) {
        case 'i':
            if (arg[2] != '\0') { handle_error("Use space after -i",arg,!(error=true)); break; }
            if (++i >= argc)    { handle_error("Missing arg for -i",NULL,!(error=true)); break; }
            if (cfg.input_file){ handle_error("Input dup",argv[i],!(error=true)); break; }
            cfg.input_file = argv[i++];
            continue;
        case 'o':
            if (arg[2] != '\0') { handle_error("Use space after -o",arg,!(error=true)); break; }
            if (++i >= argc)    { handle_error("Missing arg for -o",NULL,!(error=true)); break; }
            if (cfg.output_file){ handle_error("Output dup",argv[i],!(error=true)); break; }
            cfg.output_file = argv[i++];
            continue;
        case 'v':
            if (arg[2] != '\0') { handle_error("Use -v alone",arg,!(error=true)); break; }
            cfg.verbose = true;
            ++i;
            continue;
        default:
            handle_error("Unknown option", arg, !(error=true));
            ++i;
        }
    }

    if (!error && !cfg.input_file)
        handle_error("Missing input file", NULL, !(error=true));

    cfg.valid = !error;
    return cfg;
}

/*--------------------------------------------------------------------
 *  Helpers di basso livello
 *------------------------------------------------------------------*/
static char *safe_realloc(char *ptr, size_t new_sz) {
    char *tmp = realloc(ptr, new_sz);
    if (!tmp) {
        free(ptr);
        handle_error("realloc() failed", NULL, true);
    }
    return tmp;
}

char *read_file_content(const char *filename, long *size_out, int *lines_out) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        handle_error("Cannot open file", filename, true);
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        handle_error("fseek failed", filename, true);
    }
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        handle_error("malloc failed", filename, true);
    }
    size_t read_bytes = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[read_bytes] = '\0';
    int lines = 0;
    for (size_t i = 0; i < read_bytes; ++i)
        if (buf[i] == '\n') ++lines;
    if (read_bytes && buf[read_bytes-1] != '\n') ++lines;
    if (size_out) *size_out = (long)read_bytes;
    if (lines_out) *lines_out = lines;
    return buf;
}

char *resolve_includes(const char *code, precompiler_stats_t *st) {
    size_t capacity = strlen(code) * 2 + 1;
    char  *out      = malloc(capacity);
    if (!out) handle_error("malloc failed", NULL, true);

    size_t len_out = 0;
    const char *p = code;
    while (*p) {
        const char *start = p;
        while (*p && *p != '\n') ++p;
        const char *end = p;
        if (*p == '\n') ++p;

        size_t line_len = (size_t)(end - start);
        if (line_len >= 8 && strncmp(start, "#include", 8) == 0) {
            const char *q1 = strchr(start, '"');
            const char *q2 = q1 ? strchr(q1+1, '"') : NULL;
            if (!q1 || !q2) {
                q1 = strchr(start, '<');
                q2 = q1 ? strchr(q1+1, '>') : NULL;
            }
            if (q1 && q2 && q2 > q1 + 1) {
                char include_name[256];
                size_t copy_len = (size_t)(q2 - q1 - 1);
                if (copy_len >= sizeof include_name)
                    handle_error("include file name too long", NULL, true);
                memcpy(include_name, q1 + 1, copy_len);
                include_name[copy_len] = '\0';

                long inc_sz; int inc_ln;
                char *inc_buf      = read_file_content(include_name, &inc_sz, &inc_ln);
                char *inc_resolved = resolve_includes(inc_buf, st);

                size_t need = strlen(inc_resolved);
                if (len_out + need + 1 > capacity) {
                    capacity = (len_out + need + 1) * 2;
                    out = safe_realloc(out, capacity);
                }
                memcpy(out + len_out, inc_resolved, need);
                len_out += need;
                out[len_out] = '\0';

                st->num_files_included++;
                st->input_size  += inc_sz;
                st->input_lines += inc_ln;

                free(inc_resolved);
                free(inc_buf);
                continue;
            }
        }

        if (len_out + line_len + 1 >= capacity) {
            capacity = (len_out + line_len + 1) * 2;
            out = safe_realloc(out, capacity);
        }
        memcpy(out + len_out, start, line_len);
        len_out += line_len;
        out[len_out++] = '\n';
        out[len_out]   = '\0';
    }
    return out;
}

char *remove_comments(const char *src, precompiler_stats_t *st) {
    size_t len   = strlen(src);
    char  *dest  = malloc(len + 1);
    if (!dest) handle_error("malloc failed", NULL, true);

    enum { CODE, SLASH, LINE_COMMENT, BLOCK_COMMENT, STAR, STRING, CHARLIT } state = CODE;
    size_t i = 0, j = 0;
    while (i < len) {
        char c = src[i];
        switch (state) {
        case CODE:
            if (c == '/')          state = SLASH;
            else if (c == '"')     { dest[j++] = c; state = STRING; }
            else if (c == '\'')    { dest[j++] = c; state = CHARLIT; }
            else                   dest[j++] = c;
            ++i; break;
        case SLASH:
            if (c == '/')  { state = LINE_COMMENT; ++st->num_comments_removed; ++i; }
            else if (c == '*'){ state = BLOCK_COMMENT; ++st->num_comments_removed; ++i; }
            else { dest[j++] = '/'; state = CODE; }
            break;
        case LINE_COMMENT:
            if (c == '\n') { dest[j++] = c; state = CODE; }
            ++i; break;
        case BLOCK_COMMENT:
            if (c == '*') state = STAR;
            ++i; break;
        case STAR:
            if (c == '/') { state = CODE; ++i; }
            else          { state = BLOCK_COMMENT; }
            break;
        case STRING:
            dest[j++] = c;
            if (c == '\\' && i + 1 < len) { dest[j++] = src[++i]; }
            else if (c == '"') state = CODE;
            ++i; break;
        case CHARLIT:
            dest[j++] = c;
            if (c == '\\' && i + 1 < len) { dest[j++] = src[++i]; }
            else if (c == '\'') state = CODE;
            ++i; break;
        }
    }
    dest[j] = '\0';

    st->output_size  = (long)j;
    int lines = 0;
    for (size_t k = 0; k < j; ++k)
        if (dest[k] == '\n') ++lines;
    if (j && dest[j-1] != '\n') ++lines;
    st->output_lines = lines;

    return dest;
}

void handle_error(const char *msg, const char *file, bool fatal) {
    fprintf(stderr, "[myPreCompiler] %s", msg);
    if (file) fprintf(stderr, ": %s", file);
    if (errno) fprintf(stderr, " — %s", strerror(errno));
    fputc('\n', stderr);
    if (fatal) exit(EXIT_FAILURE);
}

void print_stats(const precompiler_stats_t *s) {
    fprintf(stderr, "Variables checked:   %d\n", s->num_variables);
    fprintf(stderr, "Invalid identifiers: %d\n", s->num_errors);
    fprintf(stderr, "Comments removed:    %d\n", s->num_comments_removed);
    fprintf(stderr, "Files included:      %d\n", s->num_files_included);
    fprintf(stderr, "Input:  %d lines,  %ld bytes\n", s->input_lines,  s->input_size);
    fprintf(stderr, "Output: %d lines,  %ld bytes\n", s->output_lines, s->output_size);
}

/*=====================================================================
 *  Helpers per il controllo degli identificatori
 *===================================================================*/
static bool is_valid_identifier(const char *id) {
    if (!id || *id == '\0') return false;
    if (!isalpha((unsigned char)id[0]) && id[0] != '_')
        return false;
    for (const char *p = id + 1; *p; ++p) {
        if (!isalnum((unsigned char)*p) && *p != '_')
            return false;
    }
    return true;
}

static bool starts_with_type_or_typedef(const char *line) {
    const char *tipi[] = {
        "int", "char", "float", "double", "long", "short", "bool",
        "unsigned", "signed", "struct", "union", "enum", "typedef",
        NULL
    };
    for (int i = 0; tipi[i]; ++i) {
        size_t len = strlen(tipi[i]);
        if (strncmp(line, tipi[i], len) == 0 && isspace((unsigned char)line[len]))
            return true;
    }
    return false;
}

static void extract_type_from_line(char *line, char *tipo_out, char **after_tipo) {
    tipo_out[0] = '\0';
    char *token = strtok(line, " \t");
    while (token) {
        if (strcmp(token, "typedef")==0 || strcmp(token,"unsigned")==0 || strcmp(token,"signed")==0 ||
            strcmp(token,"long")==0    || strcmp(token,"short")==0   || strcmp(token,"int")==0    ||
            strcmp(token,"char")==0    || strcmp(token,"float")==0   || strcmp(token,"double")==0 ||
            strcmp(token,"bool")==0    || strcmp(token,"struct")==0  || strcmp(token,"union")==0  ||
            strcmp(token,"enum")==0)
        {
            strcat(tipo_out, token);
            strcat(tipo_out, " ");
            token = strtok(NULL, " \t");
        } else {
            break;
        }
    }
    if (token)
        *after_tipo = strstr(line, token);
    else
        *after_tipo = NULL;
}

/*=====================================================================
 *  Pipeline principale di preprocessing
 *===================================================================*/
char *preprocessing_file(const char *input_file, precompiler_stats_t *stats) {
    long in_sz; int in_ln;
    char *orig = read_file_content(input_file, &in_sz, &in_ln);
    stats->input_size  = in_sz;
    stats->input_lines = in_ln;

    char *inc = resolve_includes(orig, stats);
    free(orig);

    char *no_comm = remove_comments(inc, stats);
    free(inc);

    return no_comm;
}

/*=====================================================================
 *  Validazione degli identificatori
 *===================================================================*/
void validate_identifiers(const char *code, precompiler_stats_t *stats, const char *filename) {
    char *copy = strdup(code);
    if (!copy) handle_error("strdup failed", NULL, true);

    int line_num = 1;
    char *saveptr = NULL;
    for (char *line = strtok_r(copy, "\n", &saveptr);
         line;
         line = strtok_r(NULL, "\n", &saveptr), ++line_num)
    {
        while (*line && isspace((unsigned char)*line)) ++line;

        if (strncmp(line, "#define", 7) == 0 && isspace((unsigned char)line[7])) {
            char *p = line + 7;
            while (*p && isspace((unsigned char)*p)) ++p;
            char *name = strtok(p, " \t\n");
            if (name) {
                stats->num_variables++;
                if (!is_valid_identifier(name)) {
                    stats->num_errors++;
                    fprintf(stderr, "%s:%d: invalid identifier \"%s\"\n", filename, line_num, name);
                }
            }
            continue;
        }

        if (!starts_with_type_or_typedef(line)) continue;

        char tmp[1024];
        strncpy(tmp, line, sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';

        char tipo[128];
        char *after;
        extract_type_from_line(tmp, tipo, &after);
        if (!after) continue;

        char *semi = strchr(after, ';');
        if (semi) *semi = '\0';

        char *tok_save = NULL;
        for (char *var = strtok_r(after, ",", &tok_save);
             var;
             var = strtok_r(NULL, ",", &tok_save))
        {
            while (*var && isspace((unsigned char)*var)) ++var;
            char *end = var + strlen(var) - 1;
            while (end > var && isspace((unsigned char)*end)) *end-- = '\0';

            char *eq = strchr(var, '=');
            if (eq) *eq = '\0';

            if (*var) {
                stats->num_variables++;
                if (!is_valid_identifier(var)) {
                    stats->num_errors++;
                    fprintf(stderr, "%s:%d: invalid identifier \"%s\"\n", filename, line_num, var);
                }
            }
        }
    }

    free(copy);
}