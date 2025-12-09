#include "systolic.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

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

// 测试1: 小矩阵功能测试
void test_small_matrix() {
    std::cout << "=== Test 1: Small Matrix (4x4 * 4x4) ===" << std::endl;
    
    // 创建脉动阵列
    SystolicConfig config(4, 4);
    SystolicArray array(config);
    
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
    bool success = array.matrix_multiply(A, 4, 4, B, 4, 4, C);
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
        array.print_stats();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Execution time: " << duration.count() << " us" << std::endl;
    }
}

// 测试2: 大矩阵性能测试
void test_large_matrix() {
    std::cout << "\n=== Test 2: Large Matrix (128x128 * 128x128) ===" << std::endl;
    
    SystolicConfig config(16, 16);  // 16x16 脉动阵列
    // show progress every 8 tiles to give user feedback for large runs
    config.progress_interval = 8;
    SystolicArray array(config);
    
    int M = 128, K = 128, N = 128;
    
    // 生成随机矩阵
    std::cout << "Generating random matrices..." << std::endl;
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    
    // Suppress large matrix prints to speed up the test
    // print_matrix(A, M, K, "Matrix A");
    // print_matrix(B, K, N, "Matrix B");
    
    std::vector<int32_t> C;
    
    auto start = std::chrono::high_resolution_clock::now();
    bool success = array.matrix_multiply(A, M, K, B, K, N, C);
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
        
        // 完整验证（可选，比较慢）
        bool full_correct = SystolicArray::verify_result(A, M, K, B, K, N, C);
        if (full_correct) std::cout << "Full verification PASSED!" << std::endl;
        else std::cout << "Full verification FAILED!" << std::endl;
        
        array.print_stats();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;
        
        // 计算GFLOPS
        double gflops = 2.0 * M * N * K / (duration.count() * 1e6);  // 实际是GMACs
        std::cout << "Performance: " << gflops << " GMACs/s" << std::endl;
    }
}

// 测试3: 不同数据流模式比较
void test_dataflow_modes() {
    std::cout << "\n=== Test 3: Different Dataflow Modes ===" << std::endl;
    
    int M = 64, K = 64, N = 64;
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    
    // 测试权重固定模式
    {
        std::cout << "\n1. Weight Stationary Mode:" << std::endl;
        SystolicConfig config(8, 8);
        config.dataflow = SystolicConfig::Dataflow::WEIGHT_STATIONARY;
        SystolicArray array(config);
        
        std::vector<int32_t> C;
        auto start = std::chrono::high_resolution_clock::now();
        array.matrix_multiply(A, M, K, B, K, N, C);
        auto end = std::chrono::high_resolution_clock::now();
        
        array.print_stats();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Time: " << duration.count() << " us" << std::endl;
    }
    
    // 可以类似添加输出固定和输入固定模式的测试
}

// 测试4: 不同阵列尺寸的性能缩放
void test_scaling() {
    std::cout << "\n=== Test 4: Array Size Scaling ===" << std::endl;
    
    int M = 256, K = 256, N = 256;
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    
    std::vector<int> sizes = {4, 8, 16, 32};
    
    for (int size : sizes) {
        std::cout << "\nArray size: " << size << "x" << size << std::endl;
        
        SystolicConfig config(size, size);
        SystolicArray array(config);
        
        std::vector<int32_t> C;
        auto start = std::chrono::high_resolution_clock::now();
        array.matrix_multiply(A, M, K, B, K, N, C);
        auto end = std::chrono::high_resolution_clock::now();
        
        double utilization = array.get_utilization() * 100;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Utilization: " << std::fixed << std::setprecision(1) 
                  << utilization << "%" << std::endl;
        std::cout << "Time: " << duration.count() << " ms" << std::endl;
    }
}

int main() {
    std::cout << "=== Systolic Array C Model Simulation ===" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // 运行测试
    test_small_matrix();
    test_large_matrix();
    test_dataflow_modes();
    test_scaling();
    
    std::cout << "\n=== All Tests Completed ===" << std::endl;
    
    return 0;
}