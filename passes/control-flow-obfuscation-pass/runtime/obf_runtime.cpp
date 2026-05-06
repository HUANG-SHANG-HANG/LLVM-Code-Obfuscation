///////////////////////////////////////////////////////////////////////////////
// obf_runtime.cpp
// 混淆运行时库实现
//
// 包含：
//   1. 二维整数帐篷映射（混沌子谓词）
//   2. 守护线程 + 原子变量（线程子谓词）
//   3. OR 混合谓词
//   4. 运行时初始化函数
///////////////////////////////////////////////////////////////////////////////

#include "obf_runtime.h"

#include <cstdint>
#include <atomic>
#include <thread>
#include <functional>
#include <cstdlib>

// ============================================================
// 常量
// ============================================================
static const uint32_t CHAOS_M          = 1000000;   // 帐篷映射模数
static const int      CHAOS_ITERATIONS = 10;        // 迭代次数
static const int      MAGIC_VALUE      = 42;        // 守护线程写入的魔数

// ============================================================
// 全局状态
// ============================================================

// 混沌子谓词的预计算期望值
static uint32_t g_expected_x = 0;
static uint32_t g_expected_y = 0;
static uint64_t g_chaos_seed = 0;

// 线程子谓词的共享原子变量
static std::atomic<int>  g_shared_flag{0};
static std::atomic<bool> g_stop_guardian{false};

// 初始化标志（防止重复初始化）
static std::atomic<bool> g_initialized{false};

// ============================================================
// 帐篷映射核心
// ============================================================
static uint32_t tent_step(uint32_t x) {
    int64_t val = 2 * static_cast<int64_t>(x) - static_cast<int64_t>(CHAOS_M);
    if (val < 0) val = -val;
    return static_cast<uint32_t>(static_cast<int64_t>(CHAOS_M) - val) % CHAOS_M;
}

static void chaos_iterate(uint64_t seed, uint32_t* out_x, uint32_t* out_y) {
    uint32_t x = static_cast<uint32_t>((seed >> 32) % CHAOS_M);
    uint32_t y = static_cast<uint32_t>((seed & 0xFFFFFFFF) % CHAOS_M);
    if (x == 0) x = 1;
    if (y == 0) y = 1;

    for (int i = 0; i < CHAOS_ITERATIONS; ++i) {
        x = tent_step(x);
        y = tent_step(y);
    }

    *out_x = x;
    *out_y = y;
}

// ============================================================
// 守护线程
// ============================================================
static void guardian_thread_func() {
    while (!g_stop_guardian.load(std::memory_order_relaxed)) {
        g_shared_flag.store(MAGIC_VALUE, std::memory_order_seq_cst);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// ============================================================
// 程序退出时清理守护线程
// ============================================================
static std::thread* g_guardian_thread = nullptr;

static void cleanup_guardian() {
    g_stop_guardian.store(true);
    if (g_guardian_thread && g_guardian_thread->joinable()) {
        g_guardian_thread->join();
    }
    delete g_guardian_thread;
    g_guardian_thread = nullptr;
}

// ============================================================
// 对外 API 实现
// ============================================================

extern "C" void __obf_runtime_init(void) {
    // 防止重复初始化
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true)) {
        return;
    }

    // 1. 获取种子并预计算混沌期望值
    auto tid = std::this_thread::get_id();
    g_chaos_seed = static_cast<uint64_t>(std::hash<std::thread::id>{}(tid));

    chaos_iterate(g_chaos_seed, &g_expected_x, &g_expected_y);

    // 2. 启动守护线程
    g_stop_guardian.store(false);
    g_guardian_thread = new std::thread(guardian_thread_func);

    // 3. 注册退出清理函数
    std::atexit(cleanup_guardian);
}

extern "C" bool __obf_chaos_predicate(void) {
    // 确保已初始化
    if (!g_initialized.load()) {
        __obf_runtime_init();
    }

    uint32_t cur_x, cur_y;
    chaos_iterate(g_chaos_seed, &cur_x, &cur_y);

    return (cur_x == g_expected_x) && (cur_y == g_expected_y);
}

extern "C" bool __obf_thread_predicate(void) {
    // 确保已初始化
    if (!g_initialized.load()) {
        __obf_runtime_init();
    }

    return g_shared_flag.load(std::memory_order_seq_cst) == MAGIC_VALUE;
}

extern "C" bool __obf_mixed_predicate(void) {
    bool chaos_ok  = __obf_chaos_predicate();
    bool thread_ok = __obf_thread_predicate();
    return chaos_ok || thread_ok;
}
