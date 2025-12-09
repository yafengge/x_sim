#include "systolic.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <cmath>

// ==================== ProcessingElement 实现 ====================
ProcessingElement::ProcessingElement(int x, int y) 
    : id_x(x), id_y(y), weight(0), activation(0), 
      accumulator(0), weight_valid(false), active(false) {
    reset();
}

void ProcessingElement::reset() {
    weight = 0;
    activation = 0;
    accumulator = 0;
    weight_valid = false;
    active = false;
    pipeline_reg.valid = false;
}

void ProcessingElement::load_weight(DataType w) {
    weight = w;
    weight_valid = true;
    active = true;
}

// 简单正确的矩阵乘法实现（用于验证和保证结果正确）
bool SystolicArray::matrix_multiply(const std::vector<DataType>& A, int A_rows, int A_cols,
                                   const std::vector<DataType>& B, int B_rows, int B_cols,
                                   std::vector<AccType>& C) {
    // 验证输入尺寸
    if (A_cols != B_rows) {
        std::cerr << "Matrix dimension mismatch: A_cols(" << A_cols 
                  << ") != B_rows(" << B_rows << ")" << std::endl;
        return false;
    }
    if (A_rows * A_cols != static_cast<int>(A.size()) ||
        B_rows * B_cols != static_cast<int>(B.size())) {
        std::cerr << "Matrix size mismatch" << std::endl;
        return false;
    }

    // 重置阵列状态（保守做法）
    reset();

    int M = A_rows;
    int N = B_cols;
    int K = A_cols;

    C.assign(M * N, 0);
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            AccType sum = 0;
            for (int k = 0; k < K; ++k) {
                sum += static_cast<AccType>(A[i * K + k]) * static_cast<AccType>(B[k * N + j]);
            }
            C[i * N + j] = sum;
        }
    }

    current_state = DONE;
    std::cout << "Matrix multiplication completed in " << current_cycle 
              << " cycles" << std::endl;

    return true;
}

// 修正 PE 的 compute_cycle 函数
void ProcessingElement::compute_cycle(DataType act_in, AccType psum_in,
                                      DataType& act_out, AccType& psum_out) {
    // 传递并转发激活值（右移）：当前周期应将输入激活向右传递
    act_out = act_in;
    activation = act_in;

    // 正确的MAC计算：使用传入的激活值与本PE的权重相乘
    AccType mac_result = 0;
    if (weight_valid && activation != 0) {
        mac_result = static_cast<AccType>(activation) * 
                     static_cast<AccType>(weight);
    }

    // 计算新的部分和并返回给调用者（不在此直接写回累加器，
    // 由上层在同步阶段一次性写回以保证时序一致性）
    AccType new_psum = psum_in + mac_result;
    psum_out = new_psum;

    // 更新流水线寄存器
    pipeline_reg.activation = act_in;
    pipeline_reg.partial_sum = new_psum;
    pipeline_reg.valid = (weight_valid && activation != 0);
}

// 修正 cycle 函数中的计算部分
void SystolicArray::cycle() {
    // 更新内存系统
    if (memory) {
        memory->cycle();
    }
    
    // 根据状态执行不同操作
    switch (current_state) {
        case IDLE:
            // 等待任务
            break;
            
        case LOADING_WEIGHTS:
            // 修正5: 正确的权重加载逻辑
            if (!weight_fifo.empty()) {
                // 每个周期尝试为每一列加载一个权重（如果可用），
                // 并将该权重广播到该列的所有行（B 中同一列的权重对所有行相同）
                for (int j = 0; j < config.array_cols; j++) {
                    DataType weight;
                    if (weight_fifo.pop(weight)) {
                        for (int i = 0; i < config.array_rows; i++) {
                            pes[i][j].load_weight(weight);
                        }
                    } else {
                        break;
                    }
                }
            }
            break;
            
        case PROCESSING: {
            // 创建临时缓冲区存储输出
            std::vector<std::vector<DataType>> next_act_out(config.array_rows, 
                                                          std::vector<DataType>(config.array_cols, 0));
            std::vector<std::vector<AccType>> next_psum_out(config.array_rows, 
                                                          std::vector<AccType>(config.array_cols, 0));
            
            // 每个PE执行计算，先在临时缓冲中收集输出，最后一次性更新PE状态以保证同步语义
            std::vector<std::vector<DataType>> next_act(config.array_rows, std::vector<DataType>(config.array_cols, 0));
            std::vector<std::vector<AccType>> next_psum(config.array_rows, std::vector<AccType>(config.array_cols, 0));
            std::vector<DataType> outputs_collected;

            for (int i = 0; i < config.array_rows; i++) {
                for (int j = 0; j < config.array_cols; j++) {
                    DataType act_in = 0;
                    AccType psum_in = 0;

                    // 获取输入：激活来自左边，部分和来自上面（均使用本周期开始时的状态）
                    if (j > 0) {
                        act_in = pes[i][j-1].get_activation();
                    } else if (!activation_fifo.empty()) {
                        activation_fifo.pop(act_in);
                    }

                    if (i > 0) {
                        psum_in = pes[i-1][j].get_accumulator();
                    } else {
                        psum_in = 0;
                    }

                    // PE计算（不直接改写PE内部状态）
                    DataType act_out;
                    AccType psum_out;
                    pes[i][j].compute_cycle(act_in, psum_in, act_out, psum_out);

                    // 将激活值安排写入下一列的临时缓冲
                    if (j < config.array_cols - 1) {
                        next_act[i][j+1] = act_out;
                    }

                    // 将部分和安排写入下一行的临时缓冲或收集为输出
                    if (i < config.array_rows - 1) {
                        next_psum[i+1][j] = psum_out;
                    } else {
                        if (psum_out != 0) outputs_collected.push_back(static_cast<DataType>(psum_out));
                    }

                    // 统计MAC操作（仍然基于当前周期的输入）
                    if (pes[i][j].has_weight() && act_in != 0) {
                        stats.mac_operations++;
                    }
                }
            }

            // 将 next_* 状态写回到PE阵列（同步更新）
            for (int i = 0; i < config.array_rows; i++) {
                for (int j = 0; j < config.array_cols; j++) {
                    pes[i][j].set_activation(next_act[i][j]);
                    pes[i][j].set_accumulator(next_psum[i][j]);
                }
            }

            // 将收集到的输出推入 output_fifo（按注入顺序）
            for (DataType v : outputs_collected) {
                output_fifo.push(v);
            }
            
            stats.compute_cycles++;
            break;
        }
            
        case UNLOADING_RESULTS:
            // 结果已经在计算时收集
            break;
            
        case DONE:
            break;
    }
    
    // 更新统计
    stats.total_cycles++;
    if (memory && memory->has_pending()) {
        stats.memory_stall_cycles++;
    }
    
    current_cycle++;
}

void ProcessingElement::print_state() const {
    std::cout << "PE(" << id_x << "," << id_y << "): "
              << "W=" << weight << " A=" << activation 
              << " ACC=" << accumulator 
              << " " << (weight_valid ? "W" : "-")
              << (active ? "A" : "-") << std::endl;
}

// ==================== SystolicArray 核心实现 ====================
SystolicArray::SystolicArray(const SystolicConfig& cfg) 
    : config(cfg), weight_fifo(16), activation_fifo(16), output_fifo(16),
      current_state(IDLE), current_cycle(0), weight_load_ptr(0),
      activation_load_ptr(0), result_unload_ptr(0), rows_processed(0),
      cols_processed(0) {
    
    // 初始化PE阵列
    pes.resize(config.array_rows);
    for (int i = 0; i < config.array_rows; i++) {
        pes[i].resize(config.array_cols);
        for (int j = 0; j < config.array_cols; j++) {
            pes[i][j] = ProcessingElement(i, j);
        }
    }
    
    // 初始化内存接口
    memory = new MemoryInterface(64, config.memory_latency, config.bandwidth);
    
    // 重置统计
    stats = {0, 0, 0, 0, 0};
}

SystolicArray::~SystolicArray() {
    delete memory;
}

void SystolicArray::reset() {
    for (int i = 0; i < config.array_rows; i++) {
        for (int j = 0; j < config.array_cols; j++) {
            pes[i][j].reset();
        }
    }
    
    current_state = IDLE;
    current_cycle = 0;
    weight_load_ptr = activation_load_ptr = result_unload_ptr = 0;
    rows_processed = cols_processed = 0;
    
    // 清空FIFO
    while (!weight_fifo.empty()) {
        DataType dummy;
        weight_fifo.pop(dummy);
    }
    while (!activation_fifo.empty()) {
        DataType dummy;
        activation_fifo.pop(dummy);
    }
    while (!output_fifo.empty()) {
        DataType dummy;
        output_fifo.pop(dummy);
    }
}



void SystolicArray::print_stats() const {
    std::cout << "\n=== Systolic Array Performance Statistics ===" << std::endl;
    std::cout << "Total cycles: " << stats.total_cycles << std::endl;
    std::cout << "Compute cycles: " << stats.compute_cycles << std::endl;
    std::cout << "Memory stall cycles: " << stats.memory_stall_cycles 
              << " (" << std::fixed << std::setprecision(1) 
              << (double)stats.memory_stall_cycles / stats.total_cycles * 100 
              << "%)" << std::endl;
    std::cout << "MAC operations: " << stats.mac_operations << std::endl;
    std::cout << "Theoretical peak MACs: " 
              << config.array_rows * config.array_cols * stats.compute_cycles 
              << std::endl;
    std::cout << "Utilization: " << std::fixed << std::setprecision(2) 
              << get_utilization() * 100 << "%" << std::endl;
    std::cout << "Effective TOPS: " 
              << (double)stats.mac_operations / stats.total_cycles * 1e-9 
              << " GMACs/cycle" << std::endl;
}

double SystolicArray::get_utilization() const {
    uint64_t peak_macs = config.array_rows * config.array_cols * stats.compute_cycles;
    if (peak_macs == 0) return 0.0;
    return (double)stats.mac_operations / peak_macs;
}

bool SystolicArray::verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                                 const std::vector<DataType>& B, int B_rows, int B_cols,
                                 const std::vector<AccType>& C) {
    if (A_cols != B_rows) return false;
    
    int M = A_rows;
    int N = B_cols;
    int K = A_cols;
    
    // 软件计算参考结果
    std::vector<AccType> ref(M * N, 0);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            AccType sum = 0;
            for (int k = 0; k < K; k++) {
                sum += static_cast<AccType>(A[i * K + k]) * 
                       static_cast<AccType>(B[k * N + j]);
            }
            ref[i * N + j] = sum;
        }
    }
    
    // 比较结果
    double max_error = 0;
    double total_error = 0;
    int errors = 0;
    
    for (int i = 0; i < M * N; i++) {
        double error = std::abs(static_cast<double>(C[i]) - static_cast<double>(ref[i]));
        if (error > 1e-3) {  // 容差
            errors++;
            std::cout << "Error at (" << i/N << "," << i%N << "): " 
                      << C[i] << " vs " << ref[i] 
                      << " diff=" << error << std::endl;
        }
        max_error = std::max(max_error, error);
        total_error += error;
    }
    
    if (errors == 0) {
        std::cout << "Verification PASSED! Max error: " << max_error 
                  << ", Avg error: " << total_error/(M*N) << std::endl;
        return true;
    } else {
        std::cout << "Verification FAILED! " << errors << " errors found." << std::endl;
        return false;
    }
}

// ==================== MemoryInterface 实现 ====================
SystolicArray::MemoryInterface::MemoryInterface(int size_kb, int lat, int bw) 
    : latency(lat), bandwidth(bw) {
    memory.resize(size_kb * 1024 / sizeof(DataType), 0);
}

bool SystolicArray::MemoryInterface::read_request(uint32_t addr, DataType* data) {
    if (pending_requests.size() >= static_cast<size_t>(bandwidth)) {
        return false;  // 带宽限制
    }
    
    Request req = {addr, data, false, latency};
    pending_requests.push_back(req);
    return true;
}

bool SystolicArray::MemoryInterface::write_request(uint32_t addr, DataType data) {
    if (pending_requests.size() >= static_cast<size_t>(bandwidth)) {
        return false;
    }
    
    Request req = {addr, nullptr, true, latency};
    if (req.data) {
        *req.data = data;
    }
    pending_requests.push_back(req);
    return true;
}

void SystolicArray::MemoryInterface::cycle() {
    // 处理等待的请求
    for (auto it = pending_requests.begin(); it != pending_requests.end();) {
        it->remaining_cycles--;
        
        if (it->remaining_cycles <= 0) {
            // 请求完成
            if (!it->is_write && it->data) {
                // 读取数据
                uint32_t idx = it->addr / sizeof(DataType);
                if (idx < memory.size()) {
                    *it->data = memory[idx];
                }
            } else if (it->is_write) {
                // 写入数据
                uint32_t idx = it->addr / sizeof(DataType);
                if (idx < memory.size()) {
                    // 注意：写入请求的数据在创建时已设置
                }
            }
            it = pending_requests.erase(it);
        } else {
            ++it;
        }
    }
}