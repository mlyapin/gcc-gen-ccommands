#ifndef __PLUGIN_CONFIG_H_
#define __PLUGIN_CONFIG_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct plugin_name_args;

#define CONFIG_FIRST_NDX (CONFIG_IGNORE_VERS)
#define CONFIG_LAST_NDX  (CONFIG_WANT_HELP)
#define CONFIG_COUNT     (CONFIG_LAST_NDX - CONFIG_FIRST_NDX + 1)

/**
 * Available configuration options.
 * */
enum config_opts {
        CONFIG_UNKNOWN_KEY,     /**< Used as an invalid option. */
        CONFIG_IGNORE_VERS,     /**< Do not check GCC and Janson versions at the start. */
        CONFIG_FILTER_SPECIFIC, /**< Filter out flags specific to GCC. */
        CONFIG_FILTER_INTERNAL, /**< Filter out internal GCC flags. */
        CONFIG_COMP_REPLACE,    /**< Replace a compiler name with a specified one. */
        CONFIG_OUTPUT,          /**< Specify a location of an output file. */
        CONFIG_WANT_HELP,       /**< Print help message. */
};

/**
 * One union to represent all possible values of options.
 * */
union config_values {
        /* By types. */
        bool boolean;

        /* By options. */
        bool ignore_versions;
        bool filter_specific;
        bool filter_internal;
        bool want_help;
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

/**
 * Describes an actual option.
 * */
struct config_opt {
        char const *key;         /**< It's the part after -fplugin-gen_ccommands-. */
        char const *help;        /**< Help message. */
        char const *help_defval; /**< String representation of the default value.
                                  * If NULL ptr, then the default value isn't specified. */

        bool set;                   /**< Is it set, or shall we use a default value? */
        union config_values defval; /**< Default value. */
        union config_values setval; /**< Manually set value. */
};

/**
 * @brief Parses given key and value, and applies changes to options.
 * */
enum config_opts config_apply_arg(char const *key, char const *val);

/**
 * @brief Get a value of an option.
 *
 * If the value was changed with `config_apply_arg`, the custom value will be returned,
 * otherwise you are going to get a default one.
 * */
union config_values config_get(enum config_opts opt);

/**
 * @brief Get a formatted help string.
 * */
char const *config_get_help_str(char const *plug_name);

/**
 * @brief Resets all options to their default values.
 * @note It's for unit tests actually.
 * */
void config_reset_opts(void);

#ifdef __cplusplus
}
#endif

#endif /* __PLUGIN_CONFIG_H_ */
