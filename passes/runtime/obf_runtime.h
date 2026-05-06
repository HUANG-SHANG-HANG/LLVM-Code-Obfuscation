#ifndef OBF_RUNTIME_H
#define OBF_RUNTIME_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void __obf_runtime_init(void);
bool __obf_chaos_predicate(void);
bool __obf_thread_predicate(void);
bool __obf_mixed_predicate(void);
#ifdef __cplusplus
}
#endif
#endif
