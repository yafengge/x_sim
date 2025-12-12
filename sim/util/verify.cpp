#include "util/verify.h"
#include "util/utils.h"
#include <random>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cmath>

// 文件：util/verify.cpp
// 说明：实现 verify 相关的非模板函数（如随机矩阵生成、写入并比对、软件验证逻辑等）。

namespace util {

// 生成一个 rows x cols 的随机 int16 矩阵（行主序），用于测试辅助。
std::vector<int16_t> generate_random_matrix(int rows, int cols, int min_val, int max_val) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min_val, max_val);
    std::vector<int16_t> matrix(static_cast<size_t>(rows) * static_cast<size_t>(cols));
    for (int i = 0; i < rows * cols; ++i) matrix[i] = static_cast<int16_t>(dis(gen));
    return matrix;
}

// 将 C 写出（若 cfg.c_out_path 非空），并读取 golden 文件进行比对。
bool write_and_compare(const CaseConfig &cfg, const std::vector<AccType> &C,
                       const std::vector<DataType> &A, const std::vector<DataType> &B) {
    // 先写输出文件（若请求）
    if (!cfg.c_out_path.empty()) {
        std::string c_out_resolved = util::resolve_path(cfg.c_out_path);
        if (!util::write_bin<AccType>(c_out_resolved, C)) {
            std::cerr << "write_and_compare: failed to write C_out to " << c_out_resolved << std::endl;
        }
    }

    if (cfg.c_golden_path.empty()) return true;

    std::string c_golden_resolved = util::resolve_path(cfg.c_golden_path);
    std::vector<int32_t> Cgold;
    if (!util::read_bin<int32_t>(c_golden_resolved, Cgold)) {
        std::cerr << "write_and_compare: could not read golden file " << cfg.c_golden_path << std::endl;
        return false;
    }
    if (Cgold.size() != C.size()) {
        std::cerr << "write_and_compare: golden size mismatch: " << Cgold.size() << " vs " << C.size() << std::endl;
        return false;
    }

    auto diffs_idx = compute_diffs(std::vector<int32_t>(C.begin(), C.end()), Cgold);
    if (!diffs_idx.empty()) {
        std::cerr << "write_and_compare: result does not match golden for case " << cfg.case_path << std::endl;
        print_diffs(std::vector<int32_t>(C.begin(), C.end()), Cgold);
        return false;
    }
    return true;
}

// 使用软件参考实现对 C 进行验证并打印统计信息（若失败）。
bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                   const std::vector<DataType>& B, int B_rows, int B_cols,
                   const std::vector<AccType>& C) {
    if (A_cols != B_rows) return false;

    int M = A_rows;
    int N = B_cols;
    int K = A_cols;

    if (M <= 0 || N <= 0 || K <= 0) return false;
    if ((int)A.size() != M * K) return false;
    if ((int)B.size() != K * N) return false;
    if ((int)C.size() != M * N) return false;
    
    // 使用模板 compute_diffs + print_diffs 以避免重复遍历
    std::vector<int32_t> C32(C.begin(), C.end());
    std::vector<AccType> ref_acc = compute_reference(A, M, K, B, N);
    std::vector<int32_t> ref32(ref_acc.begin(), ref_acc.end());
    auto diffs_idx = compute_diffs(C32, ref32);
    if (diffs_idx.empty()) {
        std::cout << "Verification PASSED! Max error: 0, Avg error: 0" << std::endl;
        return true;
    }

    // 仅对差异索引计算简单统计量
    double max_error = 0;
    double total_error = 0;
    for (size_t idx : diffs_idx) {
        double error = std::abs(static_cast<double>(C32[idx]) - static_cast<double>(ref32[idx]));
        max_error = std::max(max_error, error);
        total_error += error;
    }

    std::cout << "Verification FAILED! " << diffs_idx.size() << " errors found."
              << " Max error: " << max_error << ", Avg error: " << (total_error / (M * N)) << std::endl;
    print_diffs(C32, ref32);
    return false;
}

} // namespace util

