#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

char *_xstrerror(int num)
{
        return (strerror(num));
}

#pragma GCC diagnostic pop
