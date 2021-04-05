#include "paths.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#if defined(__unix__) || defined(BSD) || defined(__linux__) || defined(__APPLE__) || \
        defined(__FreeBSD__)
#define PATH_SEPARATOR '/'
#define PATH_BACKREF "/.."
#define PATH_CURREF "/."
#else
#error "Path separator isn't defined"
#endif

static char *find_nth_back_sep(char const *buffer_start, char *buffer_cur_pos, size_t nth) {
        assert(buffer_cur_pos >= buffer_start);

        while (buffer_cur_pos >= buffer_start && nth > 0) {
                if (*buffer_cur_pos == PATH_SEPARATOR) {
                        nth--;
                }
                buffer_cur_pos--;
        }

        if (nth == 0) {
                assert(buffer_cur_pos >= buffer_start);
                return (buffer_cur_pos);
        }
        return (NULL);
}

static bool find_next_seq(char const *restrict seq, char *from, char **beginning, size_t *backref_count)
{
        assert(strlen(seq) > 0);
        size_t const seq_last_ndx = strlen(seq) - 1;

        /* First iteration. */
        char *cursor = strstr(from, seq);
        if (NULL == cursor) {
                return (false);
        }
        *backref_count = 1;
        assert(seq[seq_last_ndx] == cursor[seq_last_ndx]);
        cursor = &cursor[seq_last_ndx];

        *beginning = cursor;

        while (true) {
                char *next_backref = strstr(cursor, PATH_BACKREF);

                bool const no_backrefs = NULL == next_backref;
                bool const rep_backref = next_backref - cursor == 1;
                if (no_backrefs || !rep_backref) {
                        break;
                }

                cursor = next_backref;

                (*backref_count)++;
                assert(seq[seq_last_ndx] == cursor[seq_last_ndx]);
                cursor = &cursor[seq_last_ndx];
        }

        return (true);
}


char *construct_path(char const *restrict absdir, char const *restrict relpath)
{
        assert(PATH_SEPARATOR != relpath[0]);
        assert(PATH_SEPARATOR == absdir[0]);

        char *buffer = calloc(FILENAME_MAX + 1, sizeof(*buffer));
        if (NULL == buffer) {
                return (NULL);
        }

        size_t const absdir_len = strlen(absdir);
        strcpy(buffer, absdir);
        if (PATH_SEPARATOR != buffer[absdir_len - 1]) {
                buffer[absdir_len] = PATH_SEPARATOR;
        }

        size_t const cwd_len = strnlen(buffer, FILENAME_MAX + 1);
        size_t space_remains = FILENAME_MAX + 1 - cwd_len;

        strncat(buffer, relpath, space_remains);
        size_t overall_len = strnlen(buffer, FILENAME_MAX + 1);

        char *cursor = buffer;

        /* Eliminate currefs. */
        while (true) {
                char *next_backref = NULL;
                size_t next_backref_count = 0;
                if (!find_next_seq(PATH_CURREF, cursor, &next_backref, &next_backref_count)) {
                        break;
                }

                /* Skip all the "/.." */
                char *next_valid = &next_backref[next_backref_count * (sizeof(PATH_CURREF) - 1)];
                memcpy(cursor, next_valid, strlen(next_valid));
        }

        /* Eliminate backrefs. */
        while(true) {
                char *next_backref = NULL;
                size_t next_backref_count = 0;
                if (!find_next_seq(PATH_BACKREF, cursor, &next_backref, &next_backref_count)) {
                        break;
                }

                char *back_sep = find_nth_back_sep(buffer, cursor, next_backref_count);
                if (NULL == back_sep) {
                        /* If we handle a string like "/usr/../../../../../something"
                         * then we should turn it into "/something". */
                        back_sep = buffer;
                }

                /* Skip all the "/.." */
                char *next_valid = &next_backref[next_backref_count * (sizeof(PATH_BACKREF) - 1)];
                memcpy(back_sep, next_valid, strlen(next_valid));
        }


        return (buffer);
}

#ifdef _POSIX_VERSION
char *abspath(char const *restrict relpath)
{
        char cwd[FILENAME_MAX + 1];
        if (NULL == getcwd(cwd, sizeof(cwd))) {
                return (NULL);
        }

        return (construct_path(cwd, relpath));
}
#endif

static size_t get_last_common_dirsep(char const *restrict x, char const *restrict y)
{
        size_t i = 0;
        size_t dir_ndx = 0;

        for (; x[i] != '\0' && x[i] == y[i]; i++) {
                if (x[i] == PATH_SEPARATOR) {
                        dir_ndx = i;
                }
        }

        return (dir_ndx);
}

static size_t get_remaining_dirseps(char const *path, size_t const from)
{
        size_t count = 0;

        for (size_t i = from; path[i] != '\0'; i++) {
                if (PATH_SEPARATOR == path[i]) {
                        count++;
                }
        }

        return (count);
}

char *relpath(char const *restrict filepath, char const *restrict dirpath)
{
        char *absfp = abspath(filepath);
        if (NULL == absfp) {
                goto err;
        }

        char *absdp = abspath(dirpath);
        if (NULL == absdp) {
                goto free_absfp;
        }

        size_t const absfp_len = strnlen(absfp, sizeof(absfp));
        size_t const absdp_len = strnlen(absdp, sizeof(absdp));

        size_t const last_common_dirsep = get_last_common_dirsep(absdp, absfp);
        size_t const trail_fp_len = absfp_len - last_common_dirsep;

        char *result = (char *)calloc(trail_fp_len, sizeof(*result));
        if (NULL == result) {
                goto free_absdp;
        }

        size_t const dir_seps_remains = get_remaining_dirseps(absdp, last_common_dirsep);

        for (size_t i = 0; i < dir_seps_remains; i++) {
                strncat(result, "../", trail_fp_len - 1);
        }

        strncat(result, &absfp[last_common_dirsep + 1], trail_fp_len - 1);

        return (result);

free_absdp:
        free(absdp);
free_absfp:
        free(absfp);
err:
        return (NULL);
}