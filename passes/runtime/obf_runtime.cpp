#include "obf_runtime.h"
#include <cstdint>
#include <atomic>
#include <thread>
#include <functional>
#include <cstdlib>


static const uint32_t CHAOS_M = 1000000;
static const int CHAOS_ITER  = 10;
static const int MAGIC_VALUE = 42;


static uint32_t g_expected_x = 0, g_expected_y = 0;
static uint64_t g_chaos_seed = 0;
static std::atomic<int>  g_shared_flag{0};
static std::atomic<bool> g_stop_guardian{false};
static std::atomic<bool> g_initialized{false};
static std::thread* g_guardian = nullptr;


static uint32_t tent_step(uint32_t x) {
    int64_t v = 2*(int64_t)x - (int64_t)CHAOS_M;
    if (v < 0) v = -v;
    return (uint32_t)((int64_t)CHAOS_M - v) % CHAOS_M;
}
static void chaos_iterate(uint64_t seed, uint32_t* ox, uint32_t* oy) {
    uint32_t x = (uint32_t)((seed >> 32) % CHAOS_M);
    uint32_t y = (uint32_t)((seed & 0xFFFFFFFF) % CHAOS_M);
    if (x == 0) x = 1; if (y == 0) y = 1;
    for (int i = 0; i < CHAOS_ITER; ++i) { x = tent_step(x); y = tent_step(y); }
    *ox = x; *oy = y;
}
static void guardian_func() {
    while (!g_stop_guardian.load(std::memory_order_relaxed)) {
        g_shared_flag.store(MAGIC_VALUE, std::memory_order_seq_cst);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
static void cleanup_guardian() {
    g_stop_guardian.store(true);
    if (g_guardian && g_guardian->joinable()) g_guardian->join();
    delete g_guardian; g_guardian = nullptr;
}


extern "C" void __obf_runtime_init(void) {
    bool e = false;
    if (!g_initialized.compare_exchange_strong(e, true)) return;
    auto tid = std::this_thread::get_id();
    g_chaos_seed = (uint64_t)std::hash<std::thread::id>{}(tid);
    chaos_iterate(g_chaos_seed, &g_expected_x, &g_expected_y);
    g_stop_guardian.store(false);
    g_guardian = new std::thread(guardian_func);
    std::atexit(cleanup_guardian);
}
extern "C" bool __obf_chaos_predicate(void) {
    if (!g_initialized.load()) __obf_runtime_init();
    uint32_t x, y; chaos_iterate(g_chaos_seed, &x, &y);
    return x == g_expected_x && y == g_expected_y;
}
extern "C" bool __obf_thread_predicate(void) {
    if (!g_initialized.load()) __obf_runtime_init();
    return g_shared_flag.load(std::memory_order_seq_cst) == MAGIC_VALUE;
}
extern "C" bool __obf_mixed_predicate(void) {
    return __obf_chaos_predicate() || __obf_thread_predicate();
}
