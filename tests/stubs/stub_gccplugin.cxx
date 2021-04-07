#include <cstdlib>

#include "cppdefs.h"

struct cl_decoded_option;
struct plugin_gcc_version;
typedef void (*plugin_callback_func)(void *gcc_data, void *user_data);

unsigned int save_decoded_options_count = 0;
struct cl_decoded_option *save_decoded_options = NULL;

unsigned num_in_fnames = 1;
char const *in_fnames[] = { "stub_input_file_name" };

constfn const char *get_src_pwd(void)
{
        return ("stub_src_pwd");
}

extern "C" constfn bool plugin_default_version_check(struct plugin_gcc_version *x unused,
                                                     struct plugin_gcc_version *y unused)
{
        return (true);
}

extern "C"
__attribute__((malloc))
void *xcalloc(size_t count, size_t size)
{
        return (calloc(count, size));
}

extern "C" void register_callback(const char *plugin_name unused, int event unused,
                                  plugin_callback_func callback unused, void *user_data unused)
{}
