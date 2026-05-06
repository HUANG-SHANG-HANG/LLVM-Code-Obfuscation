///////////////////////////////////////////////////////////////////////////////
// obf_runtime.h
// 混淆运行时库头文件
// Pass 注入的 call 指令会调用这些函数
///////////////////////////////////////////////////////////////////////////////

#ifndef OBF_RUNTIME_H
#define OBF_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// 初始化混淆运行时（启动守护线程、预计算混沌期望值）
/// 应在 main() 之前或入口处调用一次
void __obf_runtime_init(void);

/// 混沌子谓词：基于线程 ID 的二维整数帐篷映射
/// @return 正常运行时恒为 true
bool __obf_chaos_predicate(void);

/// 线程子谓词：读取守护线程维护的原子变量
/// @return 守护线程正常时恒为 true
bool __obf_thread_predicate(void);

/// 混合谓词：chaos_ok || thread_ok
/// @return 正常运行时恒为 true
bool __obf_mixed_predicate(void);

#ifdef __cplusplus
}
#endif

#endif // OBF_RUNTIME_H
