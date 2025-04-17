#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "precompiler.h"


/*--------------------------------------------------------------------
 *  Local helpers
 *------------------------------------------------------------------*/
static char *safe_realloc(char *ptr, size_t new_sz)
{
    char *tmp = realloc(ptr, new_sz);
    if (!tmp) {
        free(ptr);
        handle_error("realloc() failed", NULL, true);
    }
    return tmp;
}

/*--------------------------------------------------------------------
 *  Read entire file → buffer, return buffer (NUL‑terminated).
 *  Optionally returns byte size and line count.
 *------------------------------------------------------------------*/
char *read_file_content(const char *filename, long *size_out, int *lines_out)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        handle_error("Cannot open file", filename, true);
        return NULL; /* never reached if fatal */
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

    /* line count */
    int lines = 0;
    for (size_t i = 0; i < read_bytes; ++i)
        if (buf[i] == '\n') ++lines;
    if (read_bytes && buf[read_bytes-1] != '\n') ++lines; /* last line */

    if (size_out)  *size_out  = (long)read_bytes;
    if (lines_out) *lines_out = lines;

    return buf;
}

/*--------------------------------------------------------------------
 *  Resolve #include "file.h" recursively.
 *------------------------------------------------------------------*/
char *resolve_includes(const char *code, precompiler_stats_t *st)
{
    /* start with x2 size heuristic */
    size_t capacity = strlen(code) * 2 + 1;
    char  *out      = malloc(capacity);
    if (!out) handle_error("malloc failed", NULL, true);

    size_t len_out = 0;              /* current length of out string   */

    const char *p = code;
    while (*p) {
        /* read one source line */
        const char *start = p;
        while (*p && *p != '\n') ++p;
        const char *end = p; /* points to '\n' or '\0' */
        if (*p == '\n') ++p;        /* consume newline */

        size_t line_len = (size_t)(end - start);

        /* detect #include */
        if (line_len >= 8 && strncmp(start, "#include", 8) == 0) {
            const char *q1 = strchr(start, '"');
            const char *q2 = q1 ? strchr(q1+1, '"') : NULL;
            if (!q1 || !q2) {
                /* fallback to < > form */
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
                char *inc_buf = read_file_content(include_name, &inc_sz, &inc_ln);
                char *inc_resolved = resolve_includes(inc_buf, st); /* recursion */

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
                continue; /* skip original #include line */
            }
        }

        /* copy original line */
        if (len_out + line_len + 1 >= capacity) {
            capacity = (len_out + line_len + 1) * 2;
            out = safe_realloc(out, capacity);
        }
        memcpy(out + len_out, start, line_len);
        len_out += line_len;
        out[len_out++] = '\n';
        out[len_out] = '\0';
    }

    return out;
}

/*--------------------------------------------------------------------
 *  Remove comments, keeping strings & chars intact.
 *------------------------------------------------------------------*/
char *remove_comments(const char *src, precompiler_stats_t *st)
{
    size_t len   = strlen(src);
    char  *dest  = malloc(len + 1);      /* worst‑case: nothing removed */
    if (!dest) handle_error("malloc failed", NULL, true);

    enum { CODE, SLASH, LINE_COMMENT, BLOCK_COMMENT, STAR, STRING, CHARLIT } state = CODE;

    size_t i = 0, j = 0;
    while (i < len) {
        char c = src[i];
        switch (state) {
        case CODE:
            if (c == '/')          state = SLASH;
            else if (c == '"') {  dest[j++] = c; state = STRING; }
            else if (c == '\'') { dest[j++] = c; state = CHARLIT; }
            else                   dest[j++] = c;
            ++i; break;

        case SLASH:
            if (c == '/')  { state = LINE_COMMENT; ++st->num_comments_removed; ++i; }
            else if (c == '*') { state = BLOCK_COMMENT; ++st->num_comments_removed; ++i; }
            else { dest[j++] = '/'; /* previous char */ }
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
    /* line count */
    int lines = 0; for (size_t k = 0; k < j; ++k) if (dest[k] == '\n') ++lines;
    if (j && dest[j-1] != '\n') ++lines;
    st->output_lines = lines;

    return dest;
}

/*--------------------------------------------------------------------
 *  Minimal error handler (prints errno when available).
 *------------------------------------------------------------------*/
void handle_error(const char *msg, const char *file, bool fatal)
{
    fprintf(stderr, "[myPreCompiler] %s", msg);
    if (file) fprintf(stderr, ": %s", file);
    if (errno) fprintf(stderr, " — %s", strerror(errno));
    fputc('\n', stderr);
    if (fatal) exit(EXIT_FAILURE);
}

/*--------------------------------------------------------------------
 *  Verbose dump (stderr).
 *------------------------------------------------------------------*/
void print_stats(const precompiler_stats_t *s)
{
    fprintf(stderr, "Variables checked:   %d\n", s->num_variables);
    fprintf(stderr, "Invalid identifiers: %d\n", s->num_errors);
    fprintf(stderr, "Comments removed:    %d\n", s->num_comments_removed);
    fprintf(stderr, "Files included:      %d\n", s->num_files_included);
    fprintf(stderr, "Input:  %d lines,  %ld bytes\n", s->input_lines,  s->input_size);
    fprintf(stderr, "Output: %d lines,  %ld bytes\n", s->output_lines, s->output_size);
}