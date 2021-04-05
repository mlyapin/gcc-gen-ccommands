#ifndef __CHANGERS_H
#define __CHANGERS_H

#include <stdbool.h>
#include <stddef.h>

#include "arg.h"

#ifdef __cplusplus
extern "C" {
#endif

struct changer;

/**
 * @brief A function that may modify an argument or drop it.
 *
 * @c changer structure. For the private data and such.
 * @return If returns true, then pass the argument further; drop it otherwise.
 * */
typedef bool (*changer_fnt)(struct arg *arg, struct changer *c);

/**
 * Describes a changer in the changer chain.
 * */
struct changer {
        changer_fnt fn;
        void *data;
};

/**
 * Describes all active changers.
 * */
struct changer_chain {
        struct changer *changers;
        size_t changers_len;
        size_t changers_cap;
};

void changer_chain_init(struct changer_chain *cc);

void changer_chain_deinit(struct changer_chain *cc);

/**
 * @brief Append a changer to the chain.
 * */
bool changer_chain_add(struct changer_chain *cc, struct changer changer);

/**
 * @brief Feed an argument to the changer chain.
 *
 * Arguments are the same as for @changer_fnt.
 *  */
bool changer_chain_handle(struct changer_chain *cc, struct arg *arg);

bool changer_replace_compiler_with_static(struct arg *arg, struct changer *c);

bool changer_drop_internal(struct arg *arg, struct changer *c);
bool changer_drop_gccspecific(struct arg *arg, struct changer *c);

#ifdef __cplusplus
}
#endif

#endif /* __CHANGERS_H_ */
