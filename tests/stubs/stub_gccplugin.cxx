#include <gcc-plugin.h>

#include "cppdefs.h"

unsigned int save_decoded_options_count = 0;
struct cl_decoded_option *save_decoded_options = NULL;

bool plugin_default_version_check (struct plugin_gcc_version *x unused,
					  struct plugin_gcc_version *y unused)
{
        return (true);
}
