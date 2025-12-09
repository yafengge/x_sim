#ifndef SYSTOLIC_ARRAY_H
#define SYSTOLIC_ARRAY_H

#include <vector>
#include <string>
#include <iomanip>
#include <memory>

#include "systolic_common.h"
#include "processing_element.h"
#include "fifo.h"
#include "memory_interface.h"

// 脉动阵列核心
class SystolicArray {
private:
    SystolicConfig config;
    std::vector<std::vector<ProcessingElement>> pes;

    // 输入/输出FIFO（使用 shared_ptr 管理，以便 memory 请求可以持有非拥有引用）
    std::shared_ptr<FIFO> weight_fifo;
    std::shared_ptr<FIFO> activation_fifo;
    std::shared_ptr<FIFO> output_fifo;

    // 内存接口（独占所有权）
    std::unique_ptr<MemoryInterface> memory;
    
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
    
    // 性能计数器
    struct {
        uint64_t total_cycles;
        uint64_t compute_cycles;
        uint64_t memory_stall_cycles;
        uint64_t mac_operations;
        uint64_t memory_accesses;
    } stats;
    
    // 数据流控制变量
    int weight_load_ptr;
    int activation_load_ptr;
    int result_unload_ptr;
    int rows_processed;
    int cols_processed;
    
    // 控制函数
    void load_weights(const std::vector<DataType>& weights);
    void feed_activations(const std::vector<DataType>& activations);
    void collect_results(std::vector<AccType>& results);
    
    // 阵列内部数据移动
    void shift_activations_right();
    void shift_partial_sums_down();
    
public:
    SystolicArray(const SystolicConfig& cfg);
    ~SystolicArray();
    
    // 重置阵列
    void reset();
    
    // 执行矩阵乘法 C = A * B
    bool matrix_multiply(const std::vector<DataType>& A, int A_rows, int A_cols,
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