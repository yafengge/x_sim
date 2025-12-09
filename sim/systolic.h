#ifndef SYSTOLIC_ARRAY_H
#define SYSTOLIC_ARRAY_H

#include <vector>
#include <cstdint>
#include <string>
#include <iomanip>

// 数据类型定义
typedef int16_t DataType;      // 16位定点数
typedef int32_t AccType;       // 32位累加器
typedef uint64_t Cycle;        // 周期计数器

// 脉动阵列配置
struct SystolicConfig {
    int array_rows;      // 阵列行数
    int array_cols;      // 阵列列数
    int pe_latency;      // PE计算延迟（周期）
    int memory_latency;  // 内存访问延迟
    int bandwidth;       // 带宽（字/周期）
    
    // 数据流模式
    enum Dataflow {
        WEIGHT_STATIONARY,   // 权重固定（最常用）
        OUTPUT_STATIONARY,   // 输出固定
        INPUT_STATIONARY     // 输入固定
    } dataflow;
    
    SystolicConfig(int r=8, int c=8) : 
        array_rows(r), array_cols(c), pe_latency(1), 
        memory_latency(10), bandwidth(4), 
        dataflow(WEIGHT_STATIONARY) {}
};

// 处理单元（PE）状态
class ProcessingElement {
private:
    int id_x, id_y;           // PE坐标
    DataType weight;          // 存储的权重
    DataType activation;      // 当前激活值
    AccType accumulator;      // 累加器
    bool weight_valid;        // 权重是否有效
    bool active;             // PE是否激活
    
    // 流水线寄存器
    struct {
        DataType activation;
        AccType partial_sum;
        bool valid;
    } pipeline_reg;
    
public:
    ProcessingElement() = default;       

    ProcessingElement(int x, int y);
    
    // 重置PE
    void reset();
    
    // 加载权重
    void load_weight(DataType w);
    
    // 执行一个计算周期
    void compute_cycle(DataType act_in, AccType psum_in,
                       DataType& act_out, AccType& psum_out);
    
    // 获取当前累加值
    AccType get_accumulator() const { return accumulator; }
    // 激活值访问器（供外部PE间通信使用）
    DataType get_activation() const { return activation; }
    void set_activation(DataType act) { activation = act; }
    // 累加器写入器（供外部PE间通信使用）
    void set_accumulator(AccType acc) { accumulator = acc; }
    
    // PE状态查询
    bool is_active() const { return active; }
    bool has_weight() const { return weight_valid; }
    
    // 打印状态
    void print_state() const;
};

// 脉动阵列核心
class SystolicArray {
private:
    SystolicConfig config;
    std::vector<std::vector<ProcessingElement>> pes;
    
    // 输入/输出FIFO
    struct FIFO {
        std::vector<DataType> buffer;
        int depth;
        int read_ptr, write_ptr;
        int count;
        
        FIFO(int d) : depth(d), read_ptr(0), write_ptr(0), count(0) {
            buffer.resize(d);
        }
        
        bool push(DataType data) {
            if (count >= depth) return false;
            buffer[write_ptr] = data;
            write_ptr = (write_ptr + 1) % depth;
            count++;
            return true;
        }
        
        bool pop(DataType& data) {
            if (count <= 0) return false;
            data = buffer[read_ptr];
            read_ptr = (read_ptr + 1) % depth;
            count--;
            return true;
        }
        
        bool empty() const { return count == 0; }
        bool full() const { return count >= depth; }
    };
    
    // 数据队列
    FIFO weight_fifo;
    FIFO activation_fifo;
    FIFO output_fifo;
    
    // 内存接口
    class MemoryInterface {
    private:
        std::vector<DataType> memory;
        int latency;
        int bandwidth;
        struct Request {
            uint32_t addr;
            DataType* data;
            bool is_write;
            int remaining_cycles;
        };
        std::vector<Request> pending_requests;
        
    public:
        MemoryInterface(int size_kb, int lat, int bw);
        
        bool read_request(uint32_t addr, DataType* data);
        bool write_request(uint32_t addr, DataType data);
        
        void cycle();  // 每个周期调用
        bool has_pending() const { return !pending_requests.empty(); }
    };
    
    MemoryInterface* memory;
    
    // 控制器状态机
    enum State {
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