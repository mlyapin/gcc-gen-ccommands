#ifndef __SAVER_H_
#define __SAVER_H_

#include <jansson.h>

#include <stdbool.h>
#include <stdio.h>

#include "arg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Flags to customize the format of the output json file.
 * */
enum saver_flags {
        SFLAGS_NONE = 0,
        SFLAGS_STYLE_COMMAND = 0x1 << 0, /**< Generate json with "command" field
                                          * instead of "arguments". */
};

/**
 * Collects all arguments and dumps it into a file.
 * */
struct saver {
        char const *working_dir; /**< A value of the "directory" field. */
        char const *src_file;    /**< A value of the "file" field. */
        enum saver_flags flags;

        json_t *out_obj;      /**< Root dictionary object. */
        json_error_t out_err; /**< Janssons object to represent internal errors. */

        json_t *argarr; /**< When SFLAGS_STYLE_COMMAND isn't set,
                         * it points to an array object for the "arguments" field. */
        char *cmds;     /**< When SFLAGS_STYLE_COMMAND is set,
                         * it points to a temporary buffer for the "commands" field. */
        size_t cmds_len;
        size_t cmds_size;
};

/**
 * @brief Initialize an instance of the saver object.
 *
 * @warning You must call saver_deinit() when you're done to avoid memory leaks.
 * */
bool saver_init(struct saver *saver, char const *working_dir, char const *src_file,
                enum saver_flags flags);
void saver_deinit(struct saver *);

/**
 * @brief Append an argument to the "commands" or "arguments" fields.
 */
bool saver_append(struct saver *, struct arg const *arg);

/**
 * @brief Save resulted json to a file.
 * */
bool saver_save(struct saver *, char const *out_path);

/**
 * @brief Get a pointer to the last occurred error.
 *
 * - You must copy an error before continue to use saver;
 * - If there were no errors, the result is unspecified;
 */
char const *saver_get_last_err(struct saver *);

#ifdef __cplusplus
}
#endif

#endif /* __SAVER_H_ */
