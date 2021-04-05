#ifndef __SAVER_H_
#define __SAVER_H_

#include <jansson.h>

#include <stdbool.h>
#include <stdio.h>

#include "arg.h"

#ifdef __cplusplus
extern "C" {
#endif

enum saver_flags {
        SFLAGS_NONE = 0,
        SFLAGS_STYLE_COMMAND = 0x1 << 0, /**< Generate json with "command" field. */
};

struct saver {
        char const *working_dir;
        char const *src_file;
        enum saver_flags flags;

        json_t *out_obj;
        json_error_t out_err;

        /* We use argarr when SFLAGS_STYLE_COMMAND isn't set, or cmds, cmds_size and cmds_len otherwise. */
        json_t *argarr;
        char *cmds;
        size_t cmds_len;
        size_t cmds_size;
};

bool saver_init(struct saver *saver, char const *working_dir, char const *src_file,
                enum saver_flags flags);
void saver_deinit(struct saver *);

/**
 * @brief Append single option with arguments.
 */
bool saver_append(struct saver *, struct arg const *arg);

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
