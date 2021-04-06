#ifndef __ARG_H_
#define __ARG_H_

#include <stddef.h>

/** The number of possible options to an argument. */
#define ARG_MAX_NUMBER_OF_OPTS (3)

struct cl_decoded_option;

/** Describes a single argument. */
struct arg {
        enum arg_type {
                ARG_NORMAL, /**< Usual argument e.g., "-I/somedir/somefile" */
                ARG_COMPILER_NAME, /**< Compiler name. Same as argv[0]. */
                ARG_INPUT_NAME, /**< Input file. You usual source file e.g., src/saver.c */
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

/**
 * @brief Converts a decoded GCC option to our own.
 * */
struct arg convert_to_arg(struct cl_decoded_option option);

#endif /* __ARG_H_ */
