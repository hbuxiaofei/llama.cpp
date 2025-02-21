#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void alib_cpu_init(void);

void wrap__ggml_vec_dot_q4_K_q8_K(int n, float * s, size_t bs, const void *vx, size_t bx, const void *vy, size_t by, int nrc);

#ifdef __cplusplus
}
#endif
