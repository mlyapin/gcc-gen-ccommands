#ifndef __ARG_H_
#define __ARG_H_

#include <stddef.h>

#define ARG_MAX_NUMBER_OF_OPTS (3)

struct cl_decoded_option;

struct arg {
        enum arg_type {
                ARG_NORMAL,
                ARG_COMPILER_NAME,
                ARG_INPUT_NAME,
        } type;
        union {
                struct {
                        /* Let's imagine that you have such an argument "-I /usr/include".
                         * Then arg would contain "-I";
                         * opts_len would contain 1;
                         * opts would contain {"/usr/include"}
                         * */
                        char const *arg;

                        size_t opts_len;
                        char const *opts[ARG_MAX_NUMBER_OF_OPTS];
                } normal;
                /* Used as a compiler name and an input name. */
                char const *name;
        };
};

struct arg convert_to_arg(struct cl_decoded_option option);

#endif /* __ARG_H_ */
