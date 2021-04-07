#ifndef __CPPDEFS_H_
#define __CPPDEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define unused __attribute__((unused))
#define restrict __restrict__
#define ARRSIZE(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#ifdef __cplusplus
}
#endif

#endif /* __CPPDEFS_H_ */
