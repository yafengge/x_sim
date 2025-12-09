#include "systolic.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <map>

// Implementations for ProcessingElement and MemoryInterface have been
// moved to their respective compilation units: processing_element.cpp
// and memory_interface.cpp

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

    // 计数总 tile 数以便显示进度
    int tiles_total = ((M + config.array_rows - 1) / config.array_rows) *
                      ((N + config.array_cols - 1) / config.array_cols) *
                      ((K + config.array_cols - 1) / config.array_cols);
    int tiles_done = 0;

    // 分块仿真：对每个 tile 做 cycle-accurate 的 systolic 仿真（A 从左注入，B 从上注入）
    for (int mb = 0; mb < M; mb += config.array_rows) {
        int m_tile = std::min(config.array_rows, M - mb);

        for (int nb = 0; nb < N; nb += config.array_cols) {
            int n_tile = std::min(config.array_cols, N - nb);

            for (int kb = 0; kb < K; kb += config.array_cols) {
                int k_tile = std::min(config.array_cols, K - kb);

                // 本 tile 的本地寄存器：activation 在格子内向右移动，weight 向下移动，acc 为累加器
                std::vector<std::vector<DataType>> act(m_tile, std::vector<DataType>(n_tile, 0));
                std::vector<std::vector<DataType>> wt(m_tile, std::vector<DataType>(n_tile, 0));
                std::vector<std::vector<AccType>> acc(m_tile, std::vector<AccType>(n_tile, 0));

                // 计算仿真所需的安全周期数（包含 memory latency 以便数据能到达 FIFO）
                int total_cycles = k_tile + m_tile + n_tile + config.memory_latency;

                // 为本 tile 创建每行/每列的本地 FIFO，以便精确把数据注入到对应行/列
                std::vector<FIFO> localA;
                std::vector<FIFO> localB;
                for (int i = 0; i < m_tile; ++i) localA.emplace_back(k_tile + 4);
                for (int j = 0; j < n_tile; ++j) localB.emplace_back(k_tile + 4);

                // 预加载 A 和 B 到 memory 模型中（A base = 0, B base = A.size()）
                memory->load_data(A, 0);
                memory->load_data(B, static_cast<uint32_t>(A.size()));

                // 发起对本 tile 所需的所有读请求，目标直接是本地 FIFOs
                for (int kk = 0; kk < k_tile; kk++) {
                    int k_idx = kb + kk;
                    for (int i = 0; i < m_tile; ++i) {
                        uint32_t addrA = static_cast<uint32_t>((mb + i) * A_cols + (kb + kk));
                        memory->read_request(addrA, &localA[i]);
                    }
                    for (int j = 0; j < n_tile; ++j) {
                        uint32_t addrB = static_cast<uint32_t>((k_idx) * B_cols + (nb + j) + static_cast<uint32_t>(A.size()));
                        memory->read_request(addrB, &localB[j]);
                    }
                }

                // 等待本地 FIFOs 填满至少 k_tile 条数据（受 memory latency & bandwidth 影响）
                int max_wait = 10000;
                int waited = 0;
                while (waited < max_wait) {
                    bool ready = true;
                    for (int i = 0; i < m_tile; ++i) if (localA[i].count < k_tile) { ready = false; break; }
                    for (int j = 0; j < n_tile && ready; ++j) if (localB[j].count < k_tile) { ready = false; break; }
                    if (ready) break;
                    memory->cycle();
                    stats.total_cycles++; stats.memory_stall_cycles++; current_cycle++; waited++;
                }
                if (waited >= max_wait) std::cerr << "Warning: tile preload timeout" << std::endl;

                // 进行仿真处理周期：从本地 FIFOs 逐周期 pop 并 shift-then-mac
                // 为减少开销：预分配可重用的缓冲并按需清零
                std::vector<DataType> left_in(m_tile, 0);
                std::vector<DataType> top_in(n_tile, 0);
                std::vector<std::vector<DataType>> next_act(m_tile, std::vector<DataType>(n_tile, 0));
                std::vector<std::vector<DataType>> next_wt(m_tile, std::vector<DataType>(n_tile, 0));
                std::vector<std::vector<AccType>> next_acc(m_tile, std::vector<AccType>(n_tile, 0));

                for (int t = 0; t < total_cycles; t++) {
                    // 本周期根据经典注入时序决定哪些行/列需要从本地 FIFO pop
                    for (int i = 0; i < m_tile; ++i) left_in[i] = 0;
                    for (int j = 0; j < n_tile; ++j) top_in[j] = 0;

                    for (int i = 0; i < m_tile; ++i) {
                        int kk_required = t - i;
                        if (kk_required >= 0 && kk_required < k_tile) {
                            DataType v;
                            if (localA[i].pop(v)) left_in[i] = v;
                        }
                    }
                    for (int j = 0; j < n_tile; ++j) {
                        int kk_required = t - j;
                        if (kk_required >= 0 && kk_required < k_tile) {
                            DataType v;
                            if (localB[j].pop(v)) top_in[j] = v;
                        }
                    }

                    // 清零重用缓冲区
                    for (int i = 0; i < m_tile; ++i) {
                        for (int j = 0; j < n_tile; ++j) {
                            next_act[i][j] = 0;
                            next_wt[i][j] = 0;
                            next_acc[i][j] = 0;
                        }
                    }

                    uint64_t macs_this_cycle = 0;
                    for (int i = 0; i < m_tile; ++i) {
                        for (int j = 0; j < n_tile; ++j) {
                            // shift: 激活右移，权重下移
                            if (j == 0) next_act[i][j] = left_in[i];
                            else next_act[i][j] = act[i][j-1];

                            if (i == 0) next_wt[i][j] = top_in[j];
                            else next_wt[i][j] = wt[i-1][j];

                            // MAC
                            AccType mac = static_cast<AccType>(next_act[i][j]) * static_cast<AccType>(next_wt[i][j]);
                            next_acc[i][j] = acc[i][j] + mac;
                            if (next_act[i][j] != 0 && next_wt[i][j] != 0) macs_this_cycle++;
                        }
                    }

                    // 同步写回
                    act.swap(next_act);
                    wt.swap(next_wt);
                    acc.swap(next_acc);

                    // 更新统计周期
                    stats.mac_operations += macs_this_cycle;
                    stats.total_cycles++;
                    stats.compute_cycles++;
                    current_cycle++;
                }

                // 把 tile 的累加器写回 C
                for (int i = 0; i < m_tile; ++i) {
                    for (int j = 0; j < n_tile; ++j) {
                        int idx = (mb + i) * N + (nb + j);
                        C[idx] += acc[i][j];
                    }
                }
                // 更新进度计数并按需打印
                tiles_done++;
                if (config.progress_interval > 0 && (tiles_done % config.progress_interval) == 0) {
                    std::cout << "Completed " << tiles_done << " / " << tiles_total << " tiles (" \
                              << (100.0 * tiles_done / tiles_total) << "%)" << std::endl;
                }
            }
        }
    }

    current_state = DONE;
    std::cout << "Matrix multiplication completed in " << current_cycle 
              << " cycles" << std::endl;

    return true;
}

// ProcessingElement methods implemented in processing_element.cpp

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

// ProcessingElement print_state implemented in processing_element.cpp

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

// (之前在文件顶部已实现 MemoryInterface)