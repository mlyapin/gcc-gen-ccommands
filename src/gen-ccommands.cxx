#include <jansson.h>

#include <gcc-plugin.h>
#include <plugin-version.h>
#include <opts.h>
#include <toplev.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "arg.h"
#include "changers.h"
#include "cppdefs.h"
#include "paths.h"
#include "plugin_config.h"
#include "saver.h"

int plugin_is_GPL_compatible;

static void dowork(void)
{
        struct changer_chain cchain;
        changer_chain_init(&cchain);

        if (config_get(CONFIG_FILTER_INTERNAL).filter_internal) {
                changer_chain_add(&cchain, { .fn = changer_drop_internal, .data = NULL });
        }
        if (config_get(CONFIG_FILTER_SPECIFIC).filter_specific) {
                changer_chain_add(&cchain, { .fn = changer_drop_gccspecific, .data = NULL });
        }
        auto conf_compr = config_get(CONFIG_COMP_REPLACE).comp_replace;
        if (conf_compr.type == config_values::config_compreplace::CONF_COMPREPLACE_TYPE_SPECIFIED) {
                changer_chain_add(&cchain, {
                                                   .fn = changer_replace_compiler_with_static,
                                                   .data = const_cast<char *>(conf_compr.specified),
                                           });
        }

        struct saver saver;
        if (!saver_init(&saver, "./", "todo", SFLAGS_NONE)) {
                exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < save_decoded_options_count; i++) {
                struct arg a = convert_to_arg(save_decoded_options[i]);
                if (!changer_chain_handle(&cchain, &a)) {
                        continue;
                }

                if (!saver_append(&saver, &a)) {
                        exit(EXIT_FAILURE);
                }
        }

        if (!saver_save(&saver, "build/out.json")) {
                exit(EXIT_FAILURE);
        }

        saver_deinit(&saver);

        changer_chain_deinit(&cchain);
}

#define PLUGIN_ARG(KEY) "-fplugin-arg-gen-ccommands-" KEY

int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *version)
{
        for (size_t i = 0; i < info->argc; i++) {
                struct plugin_argument *argv = &info->argv[i];

                enum config_opts o = config_apply_arg(argv->key, argv->value);
                if (CONFIG_UNKNOWN_KEY == o) {
                        /* TODO Print warning about an unknown key. */
                        return (EXIT_FAILURE);
                }
        }

        bool ignore_vers = config_get(CONFIG_IGNORE_VERS).ignore_versions;
        if (!ignore_vers && !plugin_default_version_check(version, &gcc_version)) {
                fprintf(stderr,
                        "Incompatible GCC version.\n"
                        "The gen-ccommands plugin was compiled for GCC %s.\n"
                        "If you still want to continue, then append the" PLUGIN_ARG(
                                "ignore_vers") " argument to the invocation of gcc.\n",
                        basever);
                return (EXIT_FAILURE);
        }

        if (!ignore_vers && jansson_version_cmp(JANSSON_MAJOR_VERSION, JANSSON_MINOR_VERSION,
                                                JANSSON_MICRO_VERSION) != 0) {
                fprintf(stderr,
                        "Incompatible Jansson version.\n"
                        "The gen-ccommands plugin was compiled for Jansson %s, but runtime version is %s.\n"
                        "If you still want to continue, then append the" PLUGIN_ARG(
                                "ignore_vers") " argument to the invocation of gcc.\n",
                        JANSSON_VERSION, jansson_version_str());
                return (EXIT_FAILURE);
        }

        dowork();

        return (EXIT_SUCCESS);
}
