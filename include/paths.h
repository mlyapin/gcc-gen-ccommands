#ifndef __PATHS_H_
#define __PATHS_H_

#include "cppdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

char *relpath(char const *restrict filepath, char const *restrict dirpath);

char *construct_path(char const *restrict absdir, char const *restrict relpath);

char *abspath(char const *restrict relpath);

#ifdef __cplusplus
}
#endif

#endif /* __PATHS_H_ */
