#ifndef __PLUGIN_CONFIG_H_
#define __PLUGIN_CONFIG_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct plugin_name_args;

#define CONFIG_FIRST_NDX (CONFIG_IGNORE_VERS)
#define CONFIG_LAST_NDX  (CONFIG_OUTPUT)
#define CONFIG_COUNT     (CONFIG_LAST_NDX - CONFIG_FIRST_NDX + 1)

enum config_opts {
        CONFIG_UNKNOWN_KEY,
        CONFIG_IGNORE_VERS,
        CONFIG_FILTER_SPECIFIC,
        CONFIG_FILTER_INTERNAL,
        CONFIG_COMP_REPLACE,
        CONFIG_OUTPUT,
};

union config_values {
        bool ignore_versions;
        bool filter_specific;
        bool filter_internal;
        struct config_compreplace {
                enum conf_compreplace_types {
                        CONF_COMPREPLACE_TYPE_NONE,
                        CONF_COMPREPLACE_TYPE_SPECIFIED,
                } type;
                char const *specified;
        } comp_replace;
        struct config_output {
                enum conf_output_types {
                        CONF_OUT_TYPE_NEARINPUT,
                        CONF_OUT_TYPE_SPECIFIED,
                } type;
                char const *specified;
        } output;
};

struct config_opt {
        char const *key;
        char const *help;

        bool set;
        union config_values defval;
        union config_values setval;
};

enum config_opts config_apply_arg(char const *key, char const *val);

union config_values config_get(enum config_opts opt);

/* TODO Implement config_get_help. */
char const *config_get_help_str(void);

void config_reset_opts(void);

#ifdef __cplusplus
}
#endif

#endif /* __PLUGIN_CONFIG_H_ */
