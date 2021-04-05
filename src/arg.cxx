#include <gcc-plugin.h>

#include <opts.h>
#include <toplev.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "arg.h"

struct arg convert_to_arg(struct cl_decoded_option option)
{
        struct arg arg;

        if (option.opt_index == OPT_SPECIAL_program_name) {
                arg.type = arg::ARG_COMPILER_NAME;
        } else if (option.opt_index == OPT_SPECIAL_input_file) {
                arg.type = arg::ARG_INPUT_NAME;
        } else {
                arg.type = arg::ARG_NORMAL;
        }

        if (arg.type == arg::ARG_NORMAL) {
                assert(option.canonical_option_num_elements >= 1);
                assert(ARG_MAX_NUMBER_OF_OPTS + 1 >= option.canonical_option_num_elements);

                arg.normal.arg = option.canonical_option[0];

                for (size_t i = 1; i < option.canonical_option_num_elements; i++) {
                        arg.normal.opts[i - 1] = option.canonical_option[i];
                }

                /* -1 because it's always at least 1. */
                arg.normal.opts_len = option.canonical_option_num_elements - 1;
        } else {
                assert(option.canonical_option_num_elements == 1);
                arg.name = option.canonical_option[0];
        }

        return (arg);
}
