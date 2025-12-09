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

void ProcessingElement::compute_cycle(DataType act_in, AccType psum_in,
                                      DataType& act_out, AccType& psum_out) {
    // 传递激活值
    act_out = pipeline_reg.activation;
    pipeline_reg.activation = act_in;
    
    // 计算
    if (weight_valid && pipeline_reg.valid) {
        AccType mac_result = static_cast<AccType>(pipeline_reg.activation) * 
                           static_cast<AccType>(weight);
        accumulator = psum_in + mac_result;
    } else {
        accumulator = psum_in;
    }
    
    // 传递部分和
    psum_out = pipeline_reg.partial_sum;
    pipeline_reg.partial_sum = accumulator;
    pipeline_reg.valid = (act_in != 0 || weight_valid);
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
    
    // 重置阵列
    reset();
    
    int M = A_rows;
    int N = B_cols;
    int K = A_cols;  // = B_rows
    
    C.resize(M * N, 0);
    
    // 分块处理大矩阵
    for (int mb = 0; mb < M; mb += config.array_rows) {
        int m_tile = std::min(config.array_rows, M - mb);
        
        for (int nb = 0; nb < N; nb += config.array_cols) {
            int n_tile = std::min(config.array_cols, N - nb);
            
            for (int kb = 0; kb < K; kb += config.array_cols) {
                int k_tile = std::min(config.array_cols, K - kb);
                
                std::cout << "Processing tile: (" << mb << "," << nb << "," << kb 
                          << ") size: " << m_tile << "x" << n_tile << "x" << k_tile << std::endl;
                
                // 加载权重块到阵列
                current_state = LOADING_WEIGHTS;
                for (int j = 0; j < n_tile; j++) {
                    for (int k = 0; k < k_tile; k++) {
                        int b_row = kb + k;
                        int b_col = nb + j;
                        DataType weight_val = B[b_row * B_cols + b_col];
                        
                        // 加载到PE阵列的对角线
                        for (int pe_col = 0; pe_col < config.array_cols; pe_col++) {
                            if (pe_col == j && k < config.array_rows) {
                                pes[k][pe_col].load_weight(weight_val);
                            }
                        }
                    }
                }
                
                // 处理激活块
                current_state = PROCESSING;
                for (int i = 0; i < m_tile; i++) {
                    // 加载一行的激活
                    for (int k = 0; k < k_tile; k++) {
                        int a_row = mb + i;
                        int a_col = kb + k;
                        DataType act_val = A[a_row * A_cols + a_col];
                        activation_fifo.push(act_val);
                    }
                    
                    // 添加气泡（如果k_tile小于阵列宽度）
                    for (int b = k_tile; b < config.array_cols; b++) {
                        activation_fifo.push(0);
                    }
                }
                
                // 执行计算
                int total_compute_cycles = m_tile + k_tile + n_tile - 2;
                for (int cycle = 0; cycle < total_compute_cycles; cycle++) {
                    this->cycle();
                    
                    // 收集结果
                    if (cycle >= k_tile - 1) {  // 结果开始输出
                        for (int col = 0; col < n_tile; col++) {
                            AccType result = 0;
                            if (!output_fifo.empty()) {
                                DataType val;
                                output_fifo.pop(val);
                                result = static_cast<AccType>(val);
                            }
                            
                            // 累加到输出矩阵
                            int c_row = mb + (cycle - (k_tile - 1)) / n_tile;
                            int c_col = nb + col;
                            if (c_row < M && c_col < N) {
                                C[c_row * N + c_col] += result;
                            }
                        }
                    }
                }
                
                current_state = UNLOADING_RESULTS;
                // 清空剩余结果
                while (!output_fifo.empty()) {
                    DataType dummy;
                    output_fifo.pop(dummy);
                }
            }
        }
    }
    
    current_state = DONE;
    std::cout << "Matrix multiplication completed in " << current_cycle 
              << " cycles" << std::endl;
    
    return true;
}

void SystolicArray::cycle() {
    // 更新内存系统
    memory->cycle();
    
    // 根据状态执行不同操作
    switch (current_state) {
        case IDLE:
            // 等待任务
            break;
            
        case LOADING_WEIGHTS:
            // 从FIFO加载权重到PE
            for (int i = 0; i < config.array_rows; i++) {
                for (int j = 0; j < config.array_cols; j++) {
                    if (i == j && !weight_fifo.empty()) {  // 对角线加载
                        DataType weight;
                        if (weight_fifo.pop(weight)) {
                            pes[i][j].load_weight(weight);
                        }
                    }
                }
            }
            break;
            
        case PROCESSING: {
            // 脉动计算：数据在阵列中流动
            std::vector<std::vector<DataType>> act_out(config.array_rows, 
                                                      std::vector<DataType>(config.array_cols, 0));
            std::vector<std::vector<AccType>> psum_out(config.array_rows, 
                                                      std::vector<AccType>(config.array_cols, 0));
            
            // 每个PE执行计算
            for (int i = 0; i < config.array_rows; i++) {
                for (int j = 0; j < config.array_cols; j++) {
                    DataType act_in = 0;
                    AccType psum_in = 0;
                    
                    // 获取输入：激活来自左边，部分和来自上面
                    if (j > 0) act_in = act_out[i][j-1];
                    if (i > 0) psum_in = psum_out[i-1][j];
                    
                    // 第一行从FIFO获取激活
                    if (j == 0 && !activation_fifo.empty()) {
                        activation_fifo.pop(act_in);
                    }
                    
                    // 第一列的部分和初始为0
                    if (i == 0) psum_in = 0;
                    
                    // PE计算
                    DataType act_out_val;
                    AccType psum_out_val;
                    pes[i][j].compute_cycle(act_in, psum_in, 
                                          act_out_val, psum_out_val);
                    
                    act_out[i][j] = act_out_val;
                    psum_out[i][j] = psum_out_val;
                    
                    // 最后一列的输出放入输出FIFO
                    if (j == config.array_cols - 1 && psum_out_val != 0) {
                        output_fifo.push(static_cast<DataType>(psum_out_val));
                    }
                    
                    // 统计MAC操作
                    if (pes[i][j].has_weight() && act_in != 0) {
                        stats.mac_operations++;
                    }
                }
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
    if (memory->has_pending()) {
        stats.memory_stall_cycles++;
    }
    
    current_cycle++;
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