#include "changers.h"
#include "cppdefs.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

void changer_chain_init(struct changer_chain *cc)
{
        cc->changers = NULL;
        cc->changers_len = 0;
        cc->changers_cap = 0;
}

void changer_chain_deinit(struct changer_chain *cc)
{
        if (NULL != cc->changers) {
                free(cc->changers);
                cc->changers = NULL;
        }
}

bool changer_chain_add(struct changer_chain *cc, struct changer changer)
{
        if (cc->changers_len >= cc->changers_cap) {
                size_t const new_cap = cc->changers_cap > 0 ? cc->changers_cap * 2 : 1;
                struct changer *new_mem = realloc(cc->changers, new_cap * sizeof(*new_mem));
                if (NULL == new_mem) {
                        return (false);
                }

                cc->changers = new_mem;
                cc->changers_cap = new_cap;
        }

        cc->changers[cc->changers_len] = changer;
        cc->changers_len++;

        return (true);
}

bool changer_chain_handle(struct changer_chain *cc, struct arg *arg)
{
        for (size_t i = 0; i < cc->changers_len; i++) {
                struct changer *c = &cc->changers[i];

                if (!c->fn(arg, c)) {
                        return (false);
                }
        }

        return (true);
}

bool changer_replace_compiler_with_static(struct arg *arg, struct changer *c)
{
        if (arg->type != ARG_COMPILER_NAME) {
                return (true);
        }

        char const *new_comp = c->data;

        arg->name = new_comp;

        return (true);
}

static bool opts_contains_arg(size_t const opts_len, char const *opts[opts_len], char const *arg)
{
        assert(NULL != arg);

        size_t const arg_len = strlen(arg);

        for (size_t i = 0; i < opts_len; i++) {
                char const *opt = opts[i];
                size_t opt_len = strlen(opt);

                if (opt_len == 0) {
                        continue;
                }

                bool match_tail = '*' == opt[opt_len - 1];
                if (match_tail) {
                        opt_len--;
                }

                if (arg_len < opt_len) {
                        continue;
                }

                if (!match_tail && arg_len > opt_len) {
                        continue;
                }

                if (strncmp(opt, arg, opt_len) == 0) {
                        return (true);
                }
        }

        return (false);
}

static char const *GCC_INTERNAL_OPTS[] = {
        "-auxbase",
        "-dumpbase",
        "-quiet",
};
static char const *GCC_SPECIFIC_OPTS[] = {
        "-iplugindir",
        "-fplugin*",
};

bool changer_drop_internal(struct arg *arg, struct changer *f unused)
{
        assert(NULL != arg);
        if (arg->type != ARG_NORMAL) {
                return (true);
        }
        return (!opts_contains_arg(ARRSIZE(GCC_INTERNAL_OPTS), GCC_INTERNAL_OPTS, arg->normal.arg));
}

bool changer_drop_gccspecific(struct arg *arg, struct changer *f unused)
{
        assert(NULL != arg);
        if (arg->type != ARG_NORMAL) {
                return (true);
        }

        return (!opts_contains_arg(ARRSIZE(GCC_SPECIFIC_OPTS), GCC_SPECIFIC_OPTS, arg->normal.arg));
}
