#ifndef ARRAY_RUNTIME_H
#define ARRAY_RUNTIME_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// 对索引 i 进行 加密->解密 的恒等变换，但优化器无法静态化简。
// 数学上等于 i，但中间过程含 XOR、加减、乘法。
uint64_t __obf_array_idx(uint64_t i);
#ifdef __cplusplus
}
#endif
#endif
