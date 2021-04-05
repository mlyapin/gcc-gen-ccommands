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
#include "saver.h"

int plugin_is_GPL_compatible;

#define DEFAULT_IGNORE_VERSIONS     (false)
#define DEFAULT_FILTER_GCC_SPECIFIC (true)
#define DEFAULT_FILTER_GCC_INTERNAL (true)
#define DEFAULT_WORKING_DIR         (NULL)

static void dowork(void)
{
        struct changer_chain cchain;
        changer_chain_init(&cchain);

        changer_chain_add(&cchain, { .fn = changer_drop_internal, .data = NULL });
        changer_chain_add(&cchain, { .fn = changer_drop_gccspecific, .data = NULL });
        changer_chain_add(&cchain, { .fn = changer_replace_compiler_with_static,
                                     .data = const_cast<char *>("gcc") });

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
struct config {
        bool ignore_versions;
        char const *working_dir; /**< Value of the "directory" field.
                                  * If NULL, use the location of output file. */
};

static struct config gather_config(struct plugin_name_args *args)
{
        struct config c = {
                .ignore_versions = DEFAULT_IGNORE_VERSIONS,
        };

        for (size_t i = 0; i < args->argc; i++) {
                struct plugin_argument arg = args->argv[i];
#define arg_is(KEY) (strncmp(arg.key, (KEY), strlen(KEY)) == 0)

                if (arg_is("ignore_vers")) {
                        c.ignore_versions = true;
                }
        }

        return (c);
}

int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *version)
{
        struct config conf = gather_config(info);

        if (!conf.ignore_versions && !plugin_default_version_check(version, &gcc_version)) {
                fprintf(stderr,
                        "Incompatible GCC version.\n"
                        "The gen-ccommands plugin was compiled for GCC %s.\n"
                        "If you still want to continue, then append the" PLUGIN_ARG(
                                "ignore_vers") " argument to the invocation of gcc.\n",
                        basever);
                return (EXIT_FAILURE);
        }

        if (!conf.ignore_versions &&
            jansson_version_cmp(JANSSON_MAJOR_VERSION, JANSSON_MINOR_VERSION,
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
