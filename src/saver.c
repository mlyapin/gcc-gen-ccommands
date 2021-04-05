#include "saver.h"

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

/* Completely random. */
#define SAVER_CMDS_INIT_SIZE       (128u)
#define SAVER_CMDS_GROW_MULTIPLIER (2u)

static bool set_pos_at_rootarray(FILE *f)
{
        rewind(f);

        int ch = EOF;
        while ((ch = fgetc(f)) != EOF || (!feof(f))) {
                if (!isspace(ch)) {
                        break;
                }
        }

        if (ch != '[') {
                /* There is no '[' character? */
                return (false);
        }

        return (true);
}

static FILE *prepare_new(char const *json_path)
{
        FILE *f = fopen(json_path, "w+");
        if (NULL == f) {
                goto err;
        }

        assert(0 == ftell(f));
        if (EOF == fputs("[]", f)) {
                return (false);
        }

        if (!set_pos_at_rootarray(f)) {
                goto err_fclose;
        }

        return (f);

err_fclose:
        fclose(f);
err:
        return (NULL);
}

static FILE *prepare_existing(char const *json_path)
{
        FILE *f = fopen(json_path, "r+");
        if (NULL == f) {
                goto err;
        }

        long pos = ftell(f);
        if (pos < 0) {
                goto err_fclose;
        } else if (pos == 0) {
                /* The file exists, but it's empty. */
                fclose(f);
                return (prepare_new(json_path));
        }

        if (!set_pos_at_rootarray(f)) {
                goto err_fclose;
        }

        return (f);

err_fclose:
        fclose(f);
err:
        return (NULL);
}

bool saver_init(struct saver *saver, char const *json_path, char const *working_dir,
                char const *src_file, enum saver_flags flags)
{
        bool overwrite = flags & SFLAGS_OVERWRITE;
        if (access(json_path, F_OK) == 0 && !overwrite) {
                saver->outf = prepare_existing(json_path);
        } else {
                saver->outf = prepare_new(json_path);
        }

        saver->flags = flags;
#ifndef NDEBUG
        saver->saved = false;
#endif

        /* Must be either user-specified, or the plugin must determine it itself. */
        assert(NULL != working_dir);
        saver->working_dir = working_dir;

        /* There is no way we're not working on any file, right? */
        assert(NULL != src_file);
        saver->src_file = src_file;

        saver->out_obj = json_object();
        if (NULL == saver->out_obj) {
                goto err_fclose;
        }

        json_t *jworkdir = json_string(saver->working_dir);
        if (NULL == jworkdir) {
                goto err_freeobj;
        }

        json_t *jsrcfile = json_string(saver->src_file);
        if (NULL == jsrcfile) {
                goto err_free_jworkdir;
        }

        if (0 != json_object_set_new(saver->out_obj, "file", jsrcfile)) {
                goto err_free_jsrcfile;
        }

        if (0 != json_object_set_new(saver->out_obj, "directory", jworkdir)) {
                /* Previous json_object_set_new has stolen reference to jsrcfile. */
                goto err_free_jworkdir;
        }

        saver->argarr = NULL;
        saver->cmds = NULL;
        saver->cmds_len = 0;
        saver->cmds_size = 0;

        if (flags & SFLAGS_STYLE_COMMAND) {
                saver->cmds = calloc(SAVER_CMDS_INIT_SIZE, sizeof(*saver->cmds));
                if (NULL == saver->cmds) {
                        /* Previous json_object_set_new has stolen reference to jsrcfile. */
                        goto err_freeobj;
                }
                saver->cmds_size = SAVER_CMDS_INIT_SIZE;
        } else {
                saver->argarr = json_array();

                if (NULL == saver->argarr) {
                        /* Previous json_object_set_new has stolen reference to jsrcfile. */
                        goto err_freeobj;
                }

                if (0 != json_object_set_new(saver->out_obj, "arguments", saver->argarr)) {
                        goto err_free_argarr_or_cmds;
                }
        }

        return (true);

err_free_argarr_or_cmds:
        if (saver->flags & SFLAGS_STYLE_COMMAND) {
                assert(NULL != saver->cmds);
                free(saver->cmds);
        } else {
                assert(NULL != saver->argarr);
                json_decref(saver->argarr);
        }
err_free_jsrcfile:
        json_decref(jsrcfile);
err_free_jworkdir:
        json_decref(jworkdir);
err_freeobj:
        json_decref(saver->out_obj);
err_fclose:
        fclose(saver->outf);
err:
        return (false);
}

bool saver_deinit(struct saver *saver)
{
        json_decref(saver->out_obj);

        if (saver->flags & SFLAGS_STYLE_COMMAND) {
                assert(NULL == saver->argarr);
                assert(NULL != saver->cmds);
                assert(0 < saver->cmds_size);

                free(saver->cmds);
        } else {
                assert(NULL != saver->argarr);
                assert(NULL == saver->cmds);

                json_decref(saver->argarr);
        }

        return (0 == fclose(saver->outf));
}

static bool jsonarr_append_str(json_t *arr, char const *str)
{
        json_t *s = json_string(str);
        if (NULL == s) {
                return (false);
        }

        return (0 == json_array_append_new(arr, s));
}

static bool append_argarr(struct saver *saver, struct arg const *arg)
{
        assert(arg->type == ARG_NORMAL);

        if (!jsonarr_append_str(saver->argarr, arg->normal.arg)) {
                return (false);
        }

        for (size_t i = 0; i < arg->normal.opts_len; i++) {
                char const *a = arg->normal.opts[i];

                if (!jsonarr_append_str(saver->argarr, a)) {
                        return (false);
                }
        }

        return (true);
}

static bool append_to_cmdstr(struct saver *saver, char const *str)
{
        size_t const slen = strlen(str);

        /* +1 for a space separator at the end. */
        size_t const new_cmds_len = saver->cmds_len + slen + 1;

        /* Grow the cmds buffer if it's too small. */
        if (new_cmds_len > saver->cmds_size) {
                size_t new_size = saver->cmds_size;

                while (new_size < new_cmds_len) {
                        new_size *= SAVER_CMDS_GROW_MULTIPLIER;
                }

                char *new_cmds = realloc(saver->cmds, new_size);
                if (NULL == new_cmds) {
                        return (false);
                }

                saver->cmds = new_cmds;
                saver->cmds_size = new_size;
        }

        strcat(&saver->cmds[saver->cmds_len], str);
        saver->cmds_len = new_cmds_len;
        saver->cmds[saver->cmds_len - 1] = ' ';

        return (true);
}

static bool append_cmd(struct saver *saver, struct arg const *arg)
{
        assert(arg->type == ARG_NORMAL);

        if (!append_to_cmdstr(saver, arg->normal.arg)) {
                return (false);
        }

        for (size_t i = 0; i < arg->normal.opts_len; i++) {
                char const *a = arg->normal.opts[i];

                if (!append_to_cmdstr(saver, a)) {
                        return (false);
                }
        }

        return (true);
}

bool saver_append(struct saver *saver, struct arg const *arg)
{
        /* TODO Make different methods for every type. */
        if (saver->flags & SFLAGS_STYLE_COMMAND) {
                if (arg->type == ARG_NORMAL) {
                        return (append_cmd(saver, arg));
                } else {
                        return (append_to_cmdstr(saver, arg->name));
                }
        } else {
                if (arg->type == ARG_NORMAL) {
                        return (append_argarr(saver, arg));
                } else {
                        return (jsonarr_append_str(saver->argarr, arg->name));
                }
        }
}

bool saver_save(struct saver *saver)
{
#ifndef NDEBUG
        assert(!saver->saved);
        saver->saved = true;
#endif
        if (saver->flags & SFLAGS_STYLE_COMMAND) {
                json_t *jstr = json_string(saver->cmds);
                if (NULL == jstr) {
                        return (false);
                }

                json_object_set_new(saver->out_obj, "command", jstr);
        }
        return (0 == json_dumpf(saver->out_obj, saver->outf, 0));
}

char const *saver_get_last_err(struct saver *saver)
{
        if (errno != 0) {
                int e = errno;
                errno = 0;
                return (strerror(e));
        } else {
                return (saver->out_err.text);
        }
}
