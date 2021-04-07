#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <jansson.h>

#include "gcc_headers.hxx"

#include "arg.h"
#include "changers.h"
#include "cppdefs.h"
#include "paths.h"
#include "plugin_config.h"
#include "saver.h"

int plugin_is_GPL_compatible;

static char const *get_input_filename(void)
{
        if (num_in_fnames < 1) {
                return (NULL);
        } else {
                return (in_fnames[0]);
        }
}

static char const *get_output_filename(char const *input_filename)
{
        struct config_values::config_output out = config_get(CONFIG_OUTPUT).output;
        if (out.type == config_values::config_output::CONF_OUT_TYPE_SPECIFIED) {
                return (out.specified);
        } else if (out.type == config_values::config_output::CONF_OUT_TYPE_NEARINPUT) {
                char const *suffix = ".json";
                char const *ifname = input_filename;

                size_t const input_len = strlen(ifname);
                size_t const result_len = strlen(suffix) + input_len + 1;
                char *result = static_cast<char *>(xcalloc(result_len, sizeof(*result)));

                strcpy(result, ifname);
                strcat(result, suffix);

                return (result);
        } else {
                assert(false);
        }
}

static char const *get_directory(void)
{
        return (get_src_pwd());
}

static bool construct_chain(struct changer_chain *cc)
{
        changer_chain_init(cc);

        if (config_get(CONFIG_FILTER_INTERNAL).filter_internal) {
                struct changer c = { .fn = changer_drop_internal, .data = NULL };

                if (!changer_chain_add(cc, c)) {
                        goto err_deinit;
                }
        }
        if (config_get(CONFIG_FILTER_SPECIFIC).filter_specific) {
                struct changer c = { .fn = changer_drop_gccspecific, .data = NULL };

                if (!changer_chain_add(cc, c)) {
                        goto err_deinit;
                }
        }

        {
                auto comp_replacement = config_get(CONFIG_COMP_REPLACE).comp_replace;
                if (comp_replacement.type ==
                    config_values::config_compreplace::CONF_COMPREPLACE_TYPE_SPECIFIED) {
                        struct changer c = {
                                .fn = changer_replace_compiler_with_static,
                                .data = const_cast<char *>(comp_replacement.specified),
                        };

                        if (!changer_chain_add(cc, c)) {
                                goto err_deinit;
                        }
                }
        }

        return (true);

err_deinit:
        changer_chain_deinit(cc);
err:
        return (false);
}

static void deconstruct_chain(struct changer_chain *cc)
{
        changer_chain_deinit(cc);
}

static bool process_file(char const *input_fname)
{
        struct changer_chain cchain;
        struct saver saver;

        if (!construct_chain(&cchain)) {
                goto err;
        }

        if (!saver_init(&saver, get_directory(), input_fname, SFLAGS_NONE)) {
                goto err_dchain;
        }

        for (size_t i = 0; i < save_decoded_options_count; i++) {
                struct arg a = convert_to_arg(save_decoded_options[i]);
                if (!changer_chain_handle(&cchain, &a)) {
                        continue;
                }

                if (!saver_append(&saver, &a)) {
                        goto err_dsaver;
                }
        }

        if (!saver_save(&saver, get_output_filename(input_fname))) {
                goto err_dsaver;
        }

        saver_deinit(&saver);
        deconstruct_chain(&cchain);

        return (true);

err_dsaver:
        saver_deinit(&saver);
err_dchain:
        deconstruct_chain(&cchain);
err:
        return (false);
}

#define PLUGIN_ARG(KEY) "-fplugin-arg-gen-ccommands-" KEY

static struct plugin_info helpver_info = {.version = "0.0.1", .help = "Huh?"};

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

        if (config_get(CONFIG_WANT_HELP).want_help) {
                puts(config_get_help_str(info->base_name));
                return (EXIT_SUCCESS);
        }

        helpver_info.help = config_get_help_str(info->base_name);
        register_callback(info->base_name, PLUGIN_INFO, NULL, &helpver_info);

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

        const char *input_fname = get_input_filename();
        if (NULL != input_fname) {
                if (!process_file(input_fname)) {
                        return (EXIT_FAILURE);
                }
        }

        return (EXIT_SUCCESS);
}
