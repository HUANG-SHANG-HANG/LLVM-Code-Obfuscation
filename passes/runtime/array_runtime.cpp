#include "array_runtime.h"


// noinline + extern 防止优化器把 encrypt/decrypt 折叠为恒等
__attribute__((noinline))
extern "C" uint64_t __obf_array_idx(uint64_t i) {
    const uint64_t K1 = 0xCAFEBABEULL;
    const uint64_t K2 = 0x12345678ULL;
    // 加密：(i XOR K1) + K2
    volatile uint64_t enc = (i ^ K1) + K2;
    // 解密：(enc - K2) XOR K1   →   恒等于 i
    uint64_t dec = (enc - K2) ^ K1;
    return dec;
}
