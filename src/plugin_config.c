#include "plugin_config.h"
#include "cppdefs.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

static struct config_opt OPTS[] = {
                [CONFIG_UNKNOWN_KEY]   = {
                                           .key = "unknown_key",
                                           .help = "Stop right there criminal scum!"
                                                   "Nobody breaks the law on my watch!"
                                                   "Ahem. You were not supposed to see that, beware, "
                                                   "there is a bug around these parts.",
                                           .help_defval = "Huh?",
                                           .set = false,
                                         },
               [CONFIG_IGNORE_VERS]    = {
                                           .key = "ignore_ver",
                                           .help = "Disable version checks",
                                           .help_defval = "false",
                                           .set = false,
                                           .defval.ignore_versions = false,
                                         },
              [CONFIG_FILTER_SPECIFIC] = {
                                           .key = "filter_internal",
                                           .help = "Filter out GCCs internal options",
                                           .help_defval = "true",
                                           .set = false,
                                           .defval.filter_internal = true,
                                         },
              [CONFIG_FILTER_INTERNAL] = {
                                           .key = "filter_specific",
                                           .help = "Filter out GCC-only options",
                                           .help_defval = "false",
                                           .set = false,
                                           .defval.filter_specific = false,
                                         },
              [CONFIG_COMP_REPLACE]    = {
                                           .key = "replace_comp",
                                           .help = "Replace compiler entry with specified. May be <NONE>,"
                                           "which will leave compiler's name unchanged.",
                                           .help_defval = "<NONE>",
                                           .set = false,
                                           .defval.comp_replace.type = CONF_COMPREPLACE_TYPE_NONE,
                                           .defval.comp_replace.specified = NULL,
                                         },
              [CONFIG_OUTPUT]          = {
                                           .key = "output",
                                           .help = "Specify output file. May be <NEARINPUT>, "
                                           "which will create a file near an input file "
                                           "with appended .json suffix.",
                                           .help_defval = "<NEARINPUT>",
                                           .set = false,
                                           .defval.output.type = CONF_OUT_TYPE_NEARINPUT,
                                           .defval.output.specified = NULL,
                                         },
                [CONFIG_WANT_HELP]     = {
                                           .key = "help",
                                           .help = "Print this help message.",
                                           .help_defval = NULL,
                                           .set = false,
                                           .defval.want_help = false,
                                         },
};

static enum config_opts find_opt_by_key(char const *key)
{
        for (size_t i = CONFIG_FIRST_NDX; i <= CONFIG_LAST_NDX; i++) {
                struct config_opt *o = &OPTS[i];

                if (strcmp(key, o->key) == 0) {
                        return (i);
                }
        }
        return (CONFIG_UNKNOWN_KEY);
}

static void apply_val(enum config_opts opt_ndx, char const *val)
{
        struct config_opt *opt = &OPTS[opt_ndx];

        if (opt->set) {
                /* TODO Reassigning the key. Should print a warning? */
        }

        switch (opt_ndx) {
        case CONFIG_WANT_HELP:
        case CONFIG_IGNORE_VERS:
        case CONFIG_FILTER_SPECIFIC:
        case CONFIG_FILTER_INTERNAL: {
                opt->setval.filter_internal = true;
        } break;
        case CONFIG_COMP_REPLACE: {
                if (NULL == val || strcmp(val, "<NONE>") == 0) {
                        opt->setval.comp_replace.type = CONF_COMPREPLACE_TYPE_NONE;
                } else {
                        opt->setval.comp_replace.type = CONF_COMPREPLACE_TYPE_SPECIFIED;
                        opt->setval.comp_replace.specified = val;
                }
        } break;
        case CONFIG_OUTPUT: {
                if (NULL == val || strcmp(val, "<NEARINPUT>") == 0) {
                        opt->setval.output.type = CONF_OUT_TYPE_NEARINPUT;
                } else {
                        opt->setval.output.type = CONF_OUT_TYPE_SPECIFIED;
                        opt->setval.output.specified = val;
                }
        } break;
        case CONFIG_UNKNOWN_KEY:
        default:
                /* TODO Print a message instead. */
                assert(false);
        }

        opt->set = true;
}

enum config_opts config_apply_arg(char const *key, char const *val)
{
        enum config_opts opt_ndx = find_opt_by_key(key);
        if (CONFIG_UNKNOWN_KEY == opt_ndx) {
                return (opt_ndx);
        }

        apply_val(opt_ndx, val);

        return (opt_ndx);
}

union config_values config_get(enum config_opts opt_ndx)
{
        assert(opt_ndx >= CONFIG_FIRST_NDX && opt_ndx <= CONFIG_LAST_NDX);

        struct config_opt *opt = &OPTS[opt_ndx];

        if (opt->set) {
                return (opt->setval);
        } else {
                return (opt->defval);
        }
}

void config_reset_opts(void)
{
        for (size_t i = CONFIG_FIRST_NDX; i <= CONFIG_LAST_NDX; i++) {
                struct config_opt *o = &OPTS[i];
                o->set = false;
                o->setval = o->defval;
        }
}

#define HELPBUF_SIZE (2048u)
static char HELPBUF[HELPBUF_SIZE] = { 0 };
static size_t HELPBUF_POS = 0;


__attribute__((format (printf, 1, 2)))
static bool helpbuf_printf(char const *format, ...)
{
        int written = 0;

        va_list args;
        va_start(args, format);

        size_t const space_remains = HELPBUF_SIZE - HELPBUF_POS;

        written = vsnprintf(&HELPBUF[HELPBUF_POS], space_remains, format, args);
        if (written < 0) {
                goto end;
        }
        HELPBUF_POS += (size_t)written;

        if (HELPBUF_POS >= HELPBUF_SIZE) {
                assert(HELPBUF_POS < HELPBUF_SIZE);
                goto end;
        }

end:
        va_end(args);
        return (written >= 0);
}

char const *config_get_help_str(char const *plug_name)
{
        for (size_t i = CONFIG_FIRST_NDX; i <= CONFIG_LAST_NDX; i++) {
                struct config_opt *o = &OPTS[i];

                if (!helpbuf_printf("-fplugin-arg-%s-%s\n\tDescription: %s\n", plug_name, o->key,
                                    o->help)) {
                        return (NULL);
                }

                if (NULL != o->help_defval) {
                        if (!helpbuf_printf("\tDefault value: %s\n", o->help_defval)) {
                                return (NULL);
                        }
                }
        }

        return (HELPBUF);
}
