#include "systolic.h"
#include "cube.h"
#include "sim_top.h"
#include "config/config_mgr.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <random>
#include <chrono>
#include <string>
#include <iomanip>

static bool g_verbose = false;
static int g_trace_cycles = 0;
static std::string g_config_path = "config.toml";

// 生成随机矩阵
std::vector<int16_t> generate_random_matrix(int rows, int cols, int min_val = -128, int max_val = 127) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min_val, max_val);
    
    std::vector<int16_t> matrix(rows * cols);
    for (int i = 0; i < rows * cols; i++) {
        matrix[i] = static_cast<int16_t>(dis(gen));
    }
    return matrix;
}

// 打印矩阵
void print_matrix(const std::vector<int16_t>& mat, int rows, int cols, const std::string& name) {
    std::cout << "\n" << name << " (" << rows << "x" << cols << "):" << std::endl;
    for (int i = 0; i < std::min(rows, 8); i++) {  // 只打印前8行
        for (int j = 0; j < std::min(cols, 8); j++) {  // 只打印前8列
            std::cout << std::setw(6) << mat[i * cols + j] << " ";
        }
        if (cols > 8) std::cout << "...";
        std::cout << std::endl;
    }
    if (rows > 8) std::cout << "..." << std::endl;
}

// Helper: write a small TOML config file for tests (overwrites path)
static void write_config_file(const std::string& path,
                              int array_rows, int array_cols,
                              bool verbose, int trace_cycles,
                              int progress_interval,
                              Dataflow dataflow,
                              int mem_latency, int bandwidth, int max_outstanding) {
    std::ofstream fout(path);
    fout << "# Auto-generated test config\n";
    fout << "[cube]\n";
    fout << "array_rows = " << array_rows << "\n";
    fout << "array_cols = " << array_cols << "\n";
    fout << "verbose = " << (verbose ? "true" : "false") << "\n";
    fout << "trace_cycles = " << trace_cycles << "\n";
    fout << "progress_interval = " << progress_interval << "\n";
    // dataflow as string
    std::string df = "WEIGHT_STATIONARY";
    if (dataflow == Dataflow::OUTPUT_STATIONARY) df = "OUTPUT_STATIONARY";
    else if (dataflow == Dataflow::INPUT_STATIONARY) df = "INPUT_STATIONARY";
    fout << "dataflow = \"" << df << "\"\n";
    fout << "\n[memory]\n";
    fout << "memory_latency = " << mem_latency << "\n";
    fout << "bandwidth = " << bandwidth << "\n";
    fout << "max_outstanding = " << max_outstanding << "\n";
}

// 测试1: 小矩阵功能测试
void test_small_matrix() {
    std::cout << "=== Test 1: Small Matrix (4x4 * 4x4) ===" << std::endl;
    // generate a per-test config file
    write_config_file(g_config_path, 4, 4, g_verbose, g_trace_cycles, 0, Dataflow::WEIGHT_STATIONARY, 10, 4, 0);
    SimTop env(g_config_path);
    auto array = env.array();
    
    // 定义测试矩阵
    std::vector<int16_t> A = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16
    };
    
    std::vector<int16_t> B = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    std::vector<int32_t> C;
    
    auto start = std::chrono::high_resolution_clock::now();
    bool success = array->matrix_multiply(A, 4, 4, B, 4, 4, C);
    auto end = std::chrono::high_resolution_clock::now();
    
    if (success) {
        std::cout << "Result matrix C (4x4):" << std::endl;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                std::cout << std::setw(6) << C[i * 4 + j] << " ";
            }
            std::cout << std::endl;
        }
        
        // 验证结果
        bool correct = SystolicArray::verify_result(A, 4, 4, B, 4, 4, C);
        if (correct) {
            std::cout << "Test 1 PASSED!" << std::endl;
        }
        
        // 性能统计
        array->print_stats();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Execution time: " << duration.count() << " us" << std::endl;
    }
}

// 测试2: 大矩阵性能测试
void test_large_matrix(bool quick=false) {
    int M = quick ? 32 : 128;
    int K = M;
    int N = M;

    std::cout << "\n=== Test 2: Large Matrix (" << M << "x" << K << " * " << K << "x" << N << ") ===" << std::endl;
    
    // write per-test config and construct SimTop
    write_config_file(g_config_path, 16, 16, g_verbose, g_trace_cycles, quick ? 0 : 8, Dataflow::WEIGHT_STATIONARY, 10, 4, 0);
    SimTop env(g_config_path);
    auto array = env.array();
    
    // 生成随机矩阵
    std::cout << "Generating random matrices..." << std::endl;
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    
    // Suppress large matrix prints to speed up the test
    // print_matrix(A, M, K, "Matrix A");
    // print_matrix(B, K, N, "Matrix B");
    
    std::vector<int32_t> C;
    
    auto start = std::chrono::high_resolution_clock::now();
    bool success = array->matrix_multiply(A, M, K, B, K, N, C);
    auto end = std::chrono::high_resolution_clock::now();
    
    if (success) {
        // 验证部分结果
        std::cout << "\nVerifying first 4x4 block of result..." << std::endl;
        std::vector<int16_t> A_block(4 * K);
        std::vector<int16_t> B_block(K * 4);
        std::vector<int32_t> C_block(16);
        
        // 提取A的前4行
        for (int i = 0; i < 4; i++) {
            for (int k = 0; k < K; k++) {
                A_block[i * K + k] = A[i * K + k];
            }
        }
        
        // 提取B的前4列
        for (int k = 0; k < K; k++) {
            for (int j = 0; j < 4; j++) {
                B_block[k * 4 + j] = B[k * N + j];
            }
        }
        
        // 提取C的前4x4
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                C_block[i * 4 + j] = C[i * N + j];
            }
        }
        
        bool correct = SystolicArray::verify_result(A_block, 4, K, B_block, K, 4, C_block);
        if (correct) {
            std::cout << "Partial verification PASSED!" << std::endl;
        }
        
        // 完整验证（可选，较慢），在快跑模式下跳过
        if (!quick) {
            bool full_correct = SystolicArray::verify_result(A, M, K, B, K, N, C);
            if (full_correct) std::cout << "Full verification PASSED!" << std::endl;
            else std::cout << "Full verification FAILED!" << std::endl;
        } else {
            std::cout << "Full verification skipped in quick mode" << std::endl;
        }
        
        array->print_stats();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;
        
        // 计算GFLOPS
        double gflops = 2.0 * M * N * K / (duration.count() * 1e6);  // 实际是GMACs
        std::cout << "Performance: " << gflops << " GMACs/s" << std::endl;
    }
}

// 测试3: 不同数据流模式比较
void test_dataflow_modes(bool quick=false) {
    std::cout << "\n=== Test 3: Different Dataflow Modes ===" << std::endl;
    
    int M = quick ? 32 : 64;
    int K = M;
    int N = M;
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    
    // 测试权重固定模式
    {
        std::cout << "\n1. Weight Stationary Mode:" << std::endl;
        write_config_file(g_config_path, 8, 8, g_verbose, g_trace_cycles, 0, Dataflow::WEIGHT_STATIONARY, 10, 4, 0);
        SimTop env(g_config_path);
        auto array = env.array();
        
        std::vector<int32_t> C;
        auto start = std::chrono::high_resolution_clock::now();
        array->matrix_multiply(A, M, K, B, K, N, C);
        auto end = std::chrono::high_resolution_clock::now();
        
        array->print_stats();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Time: " << duration.count() << " us" << std::endl;
    }
    
    // 可以类似添加输出固定和输入固定模式的测试
}

// 测试4: 不同阵列尺寸的性能缩放
void test_scaling(bool quick=false) {
    std::cout << "\n=== Test 4: Array Size Scaling ===" << std::endl;
    
    int M = quick ? 128 : 256;
    int K = M;
    int N = M;
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    
    std::vector<int> sizes = quick ? std::vector<int>{4, 8} : std::vector<int>{4, 8, 16, 32};
    
    for (int size : sizes) {
        std::cout << "\nArray size: " << size << "x" << size << std::endl;
        
        write_config_file(g_config_path, size, size, g_verbose, g_trace_cycles, 0, Dataflow::WEIGHT_STATIONARY, 10, 4, 0);
        SimTop env(g_config_path);
        auto array = env.array();
        
        std::vector<int32_t> C;
        auto start = std::chrono::high_resolution_clock::now();
        array->matrix_multiply(A, M, K, B, K, N, C);
        auto end = std::chrono::high_resolution_clock::now();
        
        double utilization = array->get_utilization() * 100;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Utilization: " << std::fixed << std::setprecision(1) 
                  << utilization << "%" << std::endl;
        std::cout << "Time: " << duration.count() << " ms" << std::endl;
    }
}

int main(int argc, char** argv) {
    bool quick = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--quick" || arg == "-q") quick = true;
        else if (arg == "--verbose" || arg == "-v") g_verbose = true;
        else if (arg.rfind("--trace=", 0) == 0) {
            g_trace_cycles = std::stoi(arg.substr(8));
            if (g_trace_cycles > 0) g_verbose = true;
        } else if (arg.rfind("--config=", 0) == 0) {
            g_config_path = arg.substr(9);
        }
    }

    // Config is provided via `g_config_path` or tests will write test-specific
    // configs to that path prior to constructing SimTop.

    std::cout << "=== Systolic Array C Model Simulation ===" << std::endl;
    std::cout << "==========================================" << std::endl;
    if (quick) std::cout << "(Quick mode: smaller tests, skip full checks)" << std::endl;
    if (g_trace_cycles > 0) std::cout << "(Trace enabled: first " << g_trace_cycles << " cycles)" << std::endl;
    
    // 运行测试
    test_small_matrix();
    test_large_matrix(quick);
    test_dataflow_modes(quick);
    test_scaling(quick);
    
    std::cout << "\n=== All Tests Completed ===" << std::endl;
    
    return 0;
}