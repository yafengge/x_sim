#include "systolic.h"
#include "clock.h"
#include "config/config_mgr.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <map>
#include <stdexcept>

// Implementations for PE and Mem have been
// moved to their respective compilation units: pe.cpp
// and mem_if.cpp

// 简单正确的矩阵乘法实现（用于验证和保证结果正确）
bool SystolicArray::matmul(const std::vector<DataType>& A, int A_rows, int A_cols,
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
    int tiles_total = ((M + cfg_array_rows - 1) / cfg_array_rows) *
                      ((N + cfg_array_cols - 1) / cfg_array_cols) *
                      ((K + cfg_array_cols - 1) / cfg_array_cols);
    int tiles_done = 0;

    // 预先将 A/B 放入内存模型，避免每个 tile 重复加载
    memory->load_data(A, 0);
    memory->load_data(B, static_cast<uint32_t>(A.size()));

    // 预分配可复用的 FIFO 池，避免每个 tile 反复分配
    std::vector<FIFO> localA_pool(cfg_array_rows);
    std::vector<FIFO> localB_pool(cfg_array_cols);

    // 分块仿真：对每个 tile 做 cycle-accurate 的 systolic 仿真（A 从左注入，B 从上注入）
    for (int mb = 0; mb < M; mb += cfg_array_rows) {
        int m_tile = std::min(cfg_array_rows, M - mb);

        for (int nb = 0; nb < N; nb += cfg_array_cols) {
            int n_tile = std::min(cfg_array_cols, N - nb);

            for (int kb = 0; kb < K; kb += cfg_array_cols) {
                int k_tile = std::min(cfg_array_cols, K - kb);

                // 重置并配置可复用 FIFO
                for (int i = 0; i < m_tile; ++i) localA_pool[i].reset(k_tile + 4);
                for (int j = 0; j < n_tile; ++j) localB_pool[j].reset(k_tile + 4);

                // Prefetch addresses into local FIFOs (issue requests and wait)
                prefetch_tile(A, A_cols, B, B_cols, mb, nb, kb, m_tile, n_tile, k_tile, localA_pool, localB_pool);

                // Run the cycle-accurate processing for this tile and commit results into C
                // set controller state so registered SystolicArray::cycle treats these ticks as PROCESSING
                State prev_state = current_state;
                current_state = State::PROCESSING;
                process_tile(localA_pool, localB_pool, mb, nb, m_tile, n_tile, k_tile, C, N);
                current_state = prev_state;
                // 更新进度计数并按需打印
                tiles_done++;
                if (cfg_progress_interval > 0 && (tiles_done % cfg_progress_interval) == 0) {
                    std::cout << "Completed " << tiles_done << " / " << tiles_total << " tiles (" \
                              << (100.0 * tiles_done / tiles_total) << "%)" << std::endl;
                }
            }
        }
    }

    current_state = State::DONE;
    std::cout << "Matrix multiplication completed in " << current_cycle 
              << " cycles" << std::endl;

    return true;
}

// Helper: prefetch a tile's A/B data into local FIFOs
void SystolicArray::prefetch_tile(const std::vector<DataType>& A, int A_cols,
                                  const std::vector<DataType>& B, int B_cols,
                                  int mb, int nb, int kb, int m_tile, int n_tile, int k_tile,
                                  std::vector<FIFO>& localA,
                                  std::vector<FIFO>& localB) {
    // Issue read requests for all elements this tile needs into per-local completion queues.
    // We'll later drain completion queues into the local FIFOs.
    size_t queue_depth = k_tile + 4;

    // Reuse pre-allocated completion queues to避免频繁分配
    std::vector<std::shared_ptr<std::deque<DataType>>> completionA(m_tile);
    std::vector<std::shared_ptr<std::deque<DataType>>> completionB(n_tile);
    for (int i = 0; i < m_tile; ++i) {
        completionA[i] = completionA_pool[i];
        completionA[i]->clear();
    }
    for (int j = 0; j < n_tile; ++j) {
        completionB[j] = completionB_pool[j];
        completionB[j]->clear();
    }

    // A 行是连续数据，按行突发读取，减少请求数；B 按列仍逐元素读取
    for (int i = 0; i < m_tile; ++i) {
        uint32_t addrA = static_cast<uint32_t>((mb + i) * A_cols + kb);
        while (!memory->read_request(addrA, completionA[i], queue_depth, static_cast<size_t>(k_tile))) {
            stats.memory_backpressure_cycles++;
            if (clock) clock->tick();
        }
        stats.memory_accesses += static_cast<uint64_t>(k_tile);
    }
    for (int kk = 0; kk < k_tile; kk++) {
        int k_idx = kb + kk;
        for (int j = 0; j < n_tile; ++j) {
            uint32_t addrB = static_cast<uint32_t>((k_idx) * B_cols + (nb + j) + static_cast<uint32_t>(A.size()));
            while (!memory->read_request(addrB, completionB[j], queue_depth)) {
                stats.memory_backpressure_cycles++;
                if (clock) clock->tick();
            }
            stats.memory_accesses++;
        }
    }

    // Wait until each local FIFO has at least k_tile values, by draining completion queues into FIFOs as data arrives
    int max_wait = 10000;
    int waited = 0;
    while (waited < max_wait) {
        // Drain completion queues into FIFOs
        for (int i = 0; i < m_tile; ++i) {
            auto &q = completionA[i];
            while (q && !q->empty() && !localA[i].full()) {
                DataType v = q->front(); q->pop_front();
                localA[i].push(v);
            }
        }
        for (int j = 0; j < n_tile; ++j) {
            auto &q = completionB[j];
            while (q && !q->empty() && !localB[j].full()) {
                DataType v = q->front(); q->pop_front();
                localB[j].push(v);
            }
        }

        bool ready = true;
        for (int i = 0; i < m_tile; ++i) if (localA[i].count < k_tile) { ready = false; break; }
        for (int j = 0; j < n_tile && ready; ++j) if (localB[j].count < k_tile) { ready = false; break; }
        if (ready) break;
        if (clock) clock->tick();
        // after a memory cycle, try draining again (in case new completions arrived)
        for (int i = 0; i < m_tile; ++i) {
            auto &q = completionA[i];
            while (q && !q->empty() && !localA[i].full()) {
                DataType v = q->front(); q->pop_front();
                localA[i].push(v);
            }
        }
        for (int j = 0; j < n_tile; ++j) {
            auto &q = completionB[j];
            while (q && !q->empty() && !localB[j].full()) {
                DataType v = q->front(); q->pop_front();
                localB[j].push(v);
            }
        }

        waited++;
    }
    if (waited >= max_wait) std::cerr << "Warning: tile preload timeout" << std::endl;
    stats.load_cycles += waited;
}

// Helper: process a tile using its local FIFOs and commit results into C
void SystolicArray::process_tile(std::vector<FIFO>& localA,
                                 std::vector<FIFO>& localB,
                                 int mb, int nb, int m_tile, int n_tile, int k_tile,
                                 std::vector<AccType>& C, int N) {
    int mem_lat = memory ? memory->get_latency() : 0;
    int total_cycles = k_tile + m_tile + n_tile + mem_lat;

    // Reusable buffers
    std::vector<DataType> left_in(m_tile, 0);
    std::vector<DataType> top_in(n_tile, 0);

    // initialize PE state for this tile
    for (int i = 0; i < m_tile; ++i) {
        for (int j = 0; j < n_tile; ++j) {
            pes[i][j].set_accumulator(0);
            pes[i][j].set_activation(0);
        }
    }

    for (int t = 0; t < total_cycles; t++) {
        // reset inputs
        for (int i = 0; i < m_tile; ++i) left_in[i] = 0;
        for (int j = 0; j < n_tile; ++j) top_in[j] = 0;

        // Pop from local FIFOs when appropriate according to injection schedule
        for (int i = 0; i < m_tile; ++i) {
            int kk_required = t - i;
            if (kk_required >= 0 && kk_required < k_tile) {
                DataType v;
                if (localA[i].pop(v)) left_in[i] = v;
            } else {
                // otherwise feed from neighbor's last activation
            }
        }
        std::vector<char> top_valid(n_tile, 0);
        for (int j = 0; j < n_tile; ++j) {
            int kk_required = t - j;
            if (kk_required >= 0 && kk_required < k_tile) {
                DataType v;
                if (localB[j].pop(v)) { top_in[j] = v; top_valid[j] = 1; }
            }
        }

        // Prepare inputs for each PE (use current neighbor PE state for inter-PE links)
        if (cfg_verbose && m_tile <= 4 && n_tile <= 4 && t < 8) {
            std::cout << "left_in:";
            for (int ii = 0; ii < m_tile; ++ii) std::cout << " " << left_in[ii];
            std::cout << "\n";
            std::cout << "top_in:";
            for (int jj = 0; jj < n_tile; ++jj) std::cout << " " << top_in[jj];
            std::cout << "\n";
        }
        for (int i = 0; i < m_tile; ++i) {
            for (int j = 0; j < n_tile; ++j) {
                DataType act_in = 0;
                AccType psum_in = 0;
                DataType weight_in = 0;
                bool weight_present = false;
                if (j > 0) act_in = pes[i][j-1].get_activation(); else act_in = left_in[i];
                // psum is the PE's local accumulator (acc += mac each cycle)
                psum_in = pes[i][j].get_accumulator();
                if (i > 0) {
                    weight_in = pes[i-1][j].get_weight();
                    weight_present = pes[i-1][j].has_weight();
                } else {
                    weight_in = top_in[j];
                    weight_present = top_valid[j];
                }
                pes[i][j].prepare_inputs(act_in, psum_in, weight_in, weight_present);
                if (pes[i][j].has_weight() && act_in != 0) stats.mac_operations++;
            }
        }

        // Debug trace for small tiles: print staged inputs and PE state around tick
        int trace_limit = cfg_trace_cycles > 0 ? cfg_trace_cycles : 8;
        bool do_trace = (cfg_verbose && m_tile <= 4 && n_tile <= 4 && t < trace_limit);
        if (do_trace) {
            std::cout << "--- Cycle " << t << " before tick ---" << std::endl;
            for (int i = 0; i < m_tile; ++i) {
                for (int j = 0; j < n_tile; ++j) {
                    std::cout << "PE("<<i<<","<<j<<") weight="<< pes[i][j].get_weight()
                              << " act="<< pes[i][j].get_activation() << " acc="<< pes[i][j].get_accumulator() << "\n";
                }
            }
        }

        // Tick the global clock so memory->cycle, PE ticks, and SystolicArray::cycle run in order
        if (clock) clock->tick();

        if (do_trace) {
            std::cout << "--- Cycle " << t << " after tick/commit ---" << std::endl;
            for (int i = 0; i < m_tile; ++i) {
                for (int j = 0; j < n_tile; ++j) {
                    std::cout << "PE("<<i<<","<<j<<") weight="<< pes[i][j].get_weight()
                              << " act="<< pes[i][j].get_activation() << " acc="<< pes[i][j].get_accumulator() << "\n";
                }
            }
        }

        // After tick, collect outputs from bottom row as results for this cycle
        // Advance stats for compute cycle (controller's cycle will also increment total_cycles)
        stats.compute_cycles++;
    }

    // Commit accumulators from PEs to C using provided row stride N
    for (int i = 0; i < m_tile; ++i) {
        for (int j = 0; j < n_tile; ++j) {
            int idx = (mb + i) * N + (nb + j);
            AccType val = pes[i][j].get_accumulator();
            C[idx] += val;
            // debug: show committed value only when verbose is enabled to avoid huge logs
            if (cfg_verbose && m_tile <= 4 && n_tile <= 4) {
                std::cout << "Commit C("<< (mb+i) <<","<<(nb+j)<<") += "<< val <<" -> "<< C[idx] <<"\n";
            }
        }
    }
}

// PE methods implemented in pe.cpp

// 修正 cycle 函数中的计算部分
void SystolicArray::cycle() {
    // Note: memory is advanced by the global Clock listener; do not call memory->cycle() here.
    
    // 根据状态执行不同操作
    switch (current_state) {
        case State::IDLE:
            // 等待任务
            break;
            
        case State::LOADING_WEIGHTS:
            // 修正5: 正确的权重加载逻辑
            if (!weight_fifo->empty()) {
                // 每个周期尝试为每一列加载一个权重（如果可用），
                // 并将该权重广播到该列的所有行（B 中同一列的权重对所有行相同）
                for (int j = 0; j < cfg_array_cols; j++) {
                    DataType weight;
                    if (weight_fifo->pop(weight)) {
                        for (int i = 0; i < cfg_array_rows; i++) {
                            pes[i][j].load_weight(weight);
                        }
                    } else {
                        break;
                    }
                }
            }
            break;
            
        case State::PROCESSING: {
            // 创建临时缓冲区存储输出
            std::vector<std::vector<DataType>> next_act_out(cfg_array_rows, std::vector<DataType>(cfg_array_cols, 0));
            std::vector<std::vector<AccType>> next_psum_out(cfg_array_rows, std::vector<AccType>(cfg_array_cols, 0));

            // 每个PE执行计算，先在临时缓冲中收集输出，最后一次性更新PE状态以保证同步语义
            std::vector<std::vector<DataType>> next_act(cfg_array_rows, std::vector<DataType>(cfg_array_cols, 0));
            std::vector<std::vector<AccType>> next_psum(cfg_array_rows, std::vector<AccType>(cfg_array_cols, 0));
            std::vector<DataType> outputs_collected;

            // PROCESSING: now driven by per-PE ticks; here we only handle control-level actions
            // (weights are loaded in LOADING_WEIGHTS state). In tile-driven runs, process_tile
            // will prepare PE inputs and call clock->tick() so PEs execute.
            break;
        }
            
        case State::UNLOADING_RESULTS:
            // 结果已经在计算时收集
            break;
            
        case State::DONE:
            break;
    }
    
    // 更新统计
    stats.total_cycles++;
    if (memory && memory->has_pending()) {
        stats.memory_stall_cycles++;
    }
    
    current_cycle++;
}

// PE print_state implemented in pe.cpp

// ==================== SystolicArray 核心实现 ====================
SystolicArray::SystolicArray(const std::string& config_path, p_clock_t external_clock, p_mem_t external_mem)
    : config_path_(config_path),
      weight_fifo(new FIFO(16)),
      activation_fifo(new FIFO(16)),
      output_fifo(new FIFO(16)),
      current_state(State::IDLE), current_cycle(0), weight_load_ptr(0),
      activation_load_ptr(0), result_unload_ptr(0), rows_processed(0),
      cols_processed(0) {
    // load and cache configuration values once
    load_config_cache();
    
    // 初始化PE阵列 (read sizes on-demand from config file)
    pes.resize(cfg_array_rows);
    for (int i = 0; i < cfg_array_rows; i++) {
        pes[i].resize(cfg_array_cols);
        for (int j = 0; j < cfg_array_cols; j++) {
            pes[i][j] = PE(i, j);
        }
    }
    // prepare pe_listener_ids structure
    pe_listener_ids.resize(cfg_array_rows);
    for (int i = 0; i < cfg_array_rows; ++i) pe_listener_ids[i].resize(cfg_array_cols, 0);
    
    // 初始化内存接口（使用 unique_ptr）
    if (!external_mem) {
        throw std::invalid_argument("SystolicArray requires external memory; provide via SimTop::build_mem");
    }
    memory = external_mem;
    // 初始化/绑定时钟并将内存周期行为注册为监听器
    if (external_clock) {
        clock = external_clock;
    } else {
        clock = p_clock_t(new Clock());
    }
    // register memory cycle at highest priority (0)
    mem_listener_id = clock->add_listener([this]() {
        if (memory) memory->cycle();
    }, 0);
    // register PEs at priority 1 (they will run after memory but before controller)
    for (int i = 0; i < cfg_array_rows; ++i) {
        for (int j = 0; j < cfg_array_cols; ++j) {
            // capture pointer to PE
            PE* pe = &pes[i][j];
            pe_listener_ids[i][j] = clock->add_listener([pe]() { pe->tick(); }, 1);
        }
    }
    // register a commit listener at priority 2 to apply staged PE state (two-phase commit)
    commit_listener_id = clock->add_listener([this]() {
        for (int ii = 0; ii < cfg_array_rows; ++ii) {
            for (int jj = 0; jj < cfg_array_cols; ++jj) {
                pes[ii][jj].commit();
            }
        }
    }, 2);
    // register SystolicArray::cycle to be driven by clock at lower priority (3)
    sa_listener_id = clock->add_listener([this]() {
        this->cycle();
    }, 3);
    
    // 重置统计
    stats = {0, 0, 0, 0, 0, 0, 0, 0};

    // 预分配 completion 队列池
    completionA_pool.resize(cfg_array_rows);
    completionB_pool.resize(cfg_array_cols);
    for (auto &q : completionA_pool) q = std::make_shared<std::deque<DataType>>();
    for (auto &q : completionB_pool) q = std::make_shared<std::deque<DataType>>();
}

SystolicArray::~SystolicArray() {
    if (clock) {
        if (mem_listener_id) clock->remove_listener(mem_listener_id);
        if (commit_listener_id) clock->remove_listener(commit_listener_id);
        if (sa_listener_id) clock->remove_listener(sa_listener_id);
    }
}

void SystolicArray::reset() {
    for (int i = 0; i < cfg_array_rows; i++) {
        for (int j = 0; j < cfg_array_cols; j++) {
            pes[i][j].reset();
        }
    }
    
    current_state = State::IDLE;
    current_cycle = 0;
    weight_load_ptr = activation_load_ptr = result_unload_ptr = 0;
    rows_processed = cols_processed = 0;
    stats = {0, 0, 0, 0, 0, 0, 0, 0};
    
    // 清空FIFO
    while (!weight_fifo->empty()) {
        DataType dummy;
        weight_fifo->pop(dummy);
    }
    while (!activation_fifo->empty()) {
        DataType dummy;
        activation_fifo->pop(dummy);
    }
    while (!output_fifo->empty()) {
        DataType dummy;
        output_fifo->pop(dummy);
    }
}



void SystolicArray::print_stats() const {
    std::cout << "\n=== Systolic Array Performance Statistics ===" << std::endl;
    std::cout << "Total cycles: " << stats.total_cycles << std::endl;
    std::cout << "Compute cycles: " << stats.compute_cycles << std::endl;
    std::cout << "Load cycles (prefetch wait): " << stats.load_cycles << std::endl;
    std::cout << "Drain cycles: " << stats.drain_cycles << std::endl;
    std::cout << "Memory stall cycles: " << stats.memory_stall_cycles 
              << " (" << std::fixed << std::setprecision(1) 
              << (double)stats.memory_stall_cycles / stats.total_cycles * 100 
              << "%)" << std::endl;
    std::cout << "Memory backpressure cycles: " << stats.memory_backpressure_cycles << std::endl;
    std::cout << "MAC operations: " << stats.mac_operations << std::endl;
    std::cout << "Theoretical peak MACs: " 
              << (uint64_t)cfg_array_rows * (uint64_t)cfg_array_cols * stats.compute_cycles 
              << std::endl;
    std::cout << "Utilization: " << std::fixed << std::setprecision(2) 
              << get_utilization() * 100 << "%" << std::endl;
    std::cout << "Effective TOPS: " 
              << (double)stats.mac_operations / stats.total_cycles * 1e-9 
              << " GMACs/cycle" << std::endl;
}

double SystolicArray::get_utilization() const {
    uint64_t peak_macs = (uint64_t)cfg_array_rows * (uint64_t)cfg_array_cols * stats.compute_cycles;
    if (peak_macs == 0) return 0.0;
    return (double)stats.mac_operations / peak_macs;
}

int SystolicArray::cfg_int(const std::string& key, int def) const {
    int v;
    if (get_int_key(config_path_, key, v)) return v;
    return def;
}

bool SystolicArray::cfg_bool(const std::string& key, bool def) const {
    bool v;
    if (get_bool_key(config_path_, key, v)) return v;
    return def;
}

Dataflow SystolicArray::cfg_dataflow(const std::string& key, Dataflow def) const {
    Dataflow d;
    if (get_dataflow_key(config_path_, key, d)) return d;
    return def;
}

void SystolicArray::load_config_cache() {
    cfg_array_rows = cfg_int("cube.array_rows", 8);
    cfg_array_cols = cfg_int("cube.array_cols", 8);
    cfg_tile_rows = cfg_int("cube.tile_rows", cfg_array_rows);
    cfg_tile_cols = cfg_int("cube.tile_cols", cfg_array_cols);
    cfg_unroll = cfg_int("cube.unroll", 1);
    cfg_progress_interval = cfg_int("cube.progress_interval", 0);
    cfg_trace_cycles = cfg_int("cube.trace_cycles", 0);
    cfg_pe_latency = cfg_int("cube.pe_latency", 1);
    cfg_verbose = cfg_bool("cube.verbose", false);
    cfg_dataflow_cached = cfg_dataflow("cube.dataflow", Dataflow::WEIGHT_STATIONARY);
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

// (之前在文件顶部已实现 Mem)