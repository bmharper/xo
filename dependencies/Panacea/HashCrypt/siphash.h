#pragma once

#ifdef __cplusplus
extern "C" 
{
#endif

PAPI uint64_t siphash24(const void *src, size_t src_sz, const char key[16]);

#ifdef __cplusplus
}  /* end extern "C" */
#endif
