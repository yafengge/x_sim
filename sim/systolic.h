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

    // Helpers extracted from matmul for modularity
    void prefetch_tile(const std::vector<DataType>& A, int A_cols,
                       const std::vector<DataType>& B, int B_cols,
                       int mb, int nb, int kb, int m_tile, int n_tile, int k_tile,
                       std::vector<FIFO>& localA,
                       std::vector<FIFO>& localB);

    void process_tile(std::vector<FIFO>& localA,
                      std::vector<FIFO>& localB,
                      int mb, int nb, int m_tile, int n_tile, int k_tile,
                      std::vector<AccType>& C, int N);
    
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
    
    // 执行矩阵乘法 C = A * B
    bool matmul(const std::vector<DataType>& A, int A_rows, int A_cols,
                        const std::vector<DataType>& B, int B_rows, int B_cols,
                        std::vector<AccType>& C);
    
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
    
    // 验证功能
    static bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                             const std::vector<DataType>& B, int B_rows, int B_cols,
                             const std::vector<AccType>& C);
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
    std::vector<DataType>& get_active_bank() { 
        return active_bank ? bank_a : bank_b; 
    }
    std::vector<DataType>& get_inactive_bank() { 
        return active_bank ? bank_b : bank_a; 
    }
    
    void load_data(const std::vector<DataType>& data, int offset);
    std::vector<DataType> read_data(int offset, int size) const;
};

#endif // SYSTOLIC_ARRAY_H