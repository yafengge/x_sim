// systolic.h — SystolicArray 顶层声明（中文注释）
// 本头文件声明了脉动阵列（SystolicArray）以及辅助的双缓冲内存类型，
// 用于周期级仿真矩阵乘法。注释以中文提供，便于中文阅读者理解设计。
#ifndef SYSTOLIC_ARRAY_H
#define SYSTOLIC_ARRAY_H

#include <vector>
#include <string>
#include <iomanip>
#include <memory>
#include <deque>

#include "types.h"
#include "pe.h"
#include "fifo.h"
#include "mem_if.h"
#include "clock.h"

// 脉动阵列核心
class SystolicArray {
private:
    // config_path_ is used for on-demand (no-cache) config reads
    std::string config_path_;
    std::vector<std::vector<PE>> pes;

    // 输入/输出FIFO（独占所有权，由 SystolicArray 管理）
    std::unique_ptr<FIFO> weight_fifo;
    std::unique_ptr<FIFO> activation_fifo;
    std::unique_ptr<FIFO> output_fifo;

    // 内存接口（共享给外部驱动/SimTop）
    p_mem_t memory;
    
    // 控制器状态机
    enum class State {
        IDLE,
        LOADING_WEIGHTS,
        PROCESSING,
        UNLOADING_RESULTS,
        DONE
    };

    State current_state;
    Cycle current_cycle;
    // global clock for the array; components may register listeners
    p_clock_t clock;
    // listener ids registered with clock (if any)
    std::size_t mem_listener_id;
    std::size_t sa_listener_id;
    std::size_t commit_listener_id;
    // per-PE listener ids (same layout as pes)
    std::vector<std::vector<std::size_t>> pe_listener_ids;
    
    // 性能计数器
    struct {
        uint64_t total_cycles;
        uint64_t compute_cycles;
        uint64_t memory_stall_cycles;
        uint64_t mac_operations;
        uint64_t memory_accesses;
        uint64_t load_cycles;              // prefetch 等待周期
        uint64_t drain_cycles;             // 若有结果回写阶段的等待
        uint64_t memory_backpressure_cycles; // 因未完成请求过多而阻塞的周期
    } stats;
    
    // 数据流控制变量
    int weight_load_ptr;
    int activation_load_ptr;
    int result_unload_ptr;
    int rows_processed;
    int cols_processed;

    // Cached configuration values (loaded once per SystolicArray instance)
    int cfg_array_rows;
    int cfg_array_cols;
    int cfg_tile_rows;
    int cfg_tile_cols;
    int cfg_unroll;
    int cfg_progress_interval;
    int cfg_trace_cycles;
    int cfg_pe_latency;
    bool cfg_verbose;
    Dataflow cfg_dataflow_cached;

    // Load configuration values from file into cached members
    void load_config_cache();
    
    // 控制函数
    void load_weights(const std::vector<DataType>& weights);
    void feed_activations(const std::vector<DataType>& activations);
    void collect_results(std::vector<AccType>& results);
    
    // 阵列内部数据移动
    void shift_activations_right();
    void shift_partial_sums_down();

    // 复用的 completion 队列池，避免重复分配
    std::vector<std::shared_ptr<std::deque<DataType>>> completionA_pool;
    std::vector<std::shared_ptr<std::deque<DataType>>> completionB_pool;

    // Helpers for tile execution (small, single-responsibility)
    void init_tile_state(int m_tile, int n_tile);
    bool execute_tile_cycles(std::vector<FIFO>& localA_pool,
                             std::vector<FIFO>& localB_pool,
                             int m_tile, int n_tile, int k_tile);
    void commit_tile_results(int mb, int nb, int m_tile, int n_tile,
                             uint32_t c_addr, int N);

    // New helpers to support memory-driven runs.
    bool issue_prefetch_for_tile(int mb, int nb, int kb, int m_tile, int n_tile, int k_tile,
                                 int A_cols, int B_cols,
                                 uint32_t a_addr, uint32_t b_addr,
                                 std::vector<FIFO>& localA_pool,
                                 std::vector<FIFO>& localB_pool,
                                 std::vector<std::shared_ptr<std::deque<DataType>>>& completionA,
                                 std::vector<std::shared_ptr<std::deque<DataType>>>& completionB,
                                 size_t queue_depth);

    bool wait_for_prefetch(int m_tile, int n_tile, int k_tile,
                           std::vector<FIFO>& localA_pool,
                           std::vector<FIFO>& localB_pool);

    bool process_tile(std::vector<FIFO>& localA_pool,
                                  std::vector<FIFO>& localB_pool,
                                  int mb, int nb, int m_tile, int n_tile, int k_tile,
                                  uint32_t c_addr, int N);
    
public:
    SystolicArray(const std::string& config_path,
                  p_clock_t external_clock,
                  p_mem_t external_mem = nullptr);

    // helpers that read configuration keys from config file (no caching)
    int cfg_int(const std::string& key, int def) const;
    bool cfg_bool(const std::string& key, bool def) const;
    Dataflow cfg_dataflow(const std::string& key, Dataflow def) const;
    ~SystolicArray();
    
    // 重置阵列
    void reset();
    
    // (旧接口已移除) 矩阵乘法请使用 `run` 或者通过 `Cube::run` 入口

    // Run the array using data already present in `memory` at the provided
    // addresses. The array will issue read requests to `memory` and commit
    // accumulator results back into memory via `store_acc_direct` at offsets
    // starting at `c_addr`.
    bool run(int M, int N, int K,
                         uint32_t a_addr, uint32_t b_addr, uint32_t c_addr);
    
    // 单周期推进
    void cycle();
    
    // 获取状态
    State get_state() const { return current_state; }
    Cycle get_cycle() const { return current_cycle; }
    
    // 性能统计
    void print_stats() const;
    double get_utilization() const;
    double get_memory_efficiency() const;
    
    // 调试功能
    void print_array_state() const;
    void enable_tracing(const std::string& filename);

    // 访问内存模型（供封装层获取共享指针）
    p_mem_t get_memory() { return memory; }
    p_mem_t get_memory() const { return memory; }
    
    // 验证功能已迁移到 util/verify.{h,cpp} 中作为独立工具函数。
    // 若仍需通过类访问，请使用 SystolicArray::verify_result (已向后兼容直到下一次重大版本移除)。
};

// 内存模型
class DoubleBufferedMemory {
private:
    std::vector<DataType> bank_a;
    std::vector<DataType> bank_b;
    bool active_bank;  // true=A, false=B
    int buffer_size;
    
public:
    DoubleBufferedMemory(int size);
    
    void switch_bank() { active_bank = !active_bank; }
    // Buffer accessors removed: not used in project API surface.
    
    void load_data(const std::vector<DataType>& data, int offset);
    std::vector<DataType> read_data(int offset, int size) const;
};

#endif // SYSTOLIC_ARRAY_H