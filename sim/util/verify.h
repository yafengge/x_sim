#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <iostream>
#include "util/log.h"
#include "types.h"
#include "util/case_io.h"

namespace util {

// 计算两个向量之间不同元素的索引（模板，返回差异索引列表）
template<typename T>
std::vector<size_t> compute_diffs(const std::vector<T>& got, const std::vector<T>& expected);

// 打印差异（最多打印 `max_print` 条）。若提供 `indices`，仅打印这些索引。
template<typename T>
void print_diffs(const std::vector<T>& got, const std::vector<T>& expected,
                 const std::vector<size_t>* indices = nullptr, size_t max_print = 20) {
    size_t n = std::min(got.size(), expected.size());
    size_t diffs_printed = 0;
    if (indices) {
        for (size_t idx : *indices) {
            if (diffs_printed >= max_print) break;
            T g = (idx < got.size()) ? got[idx] : T();
            T e = (idx < expected.size()) ? expected[idx] : T();
            LOG_INFO("diff at {}: expected={} got={}", idx, e, g);
            ++diffs_printed;
        }
    } else {
        for (size_t i = 0; i < n && diffs_printed < max_print; ++i) {
            if (got[i] != expected[i]) {
                LOG_INFO("diff at {}: expected={} got={}", i, expected[i], got[i]);
                ++diffs_printed;
            }
        }
    }
    if (got.size() != expected.size()) {
        LOG_WARN("size mismatch: got={} golden={}", got.size(), expected.size());
    }
    // 打印总差异数
    auto total_diffs = compute_diffs<T>(got, expected);
    LOG_INFO("total differences: {}", total_diffs.size());
}

// 将累加器 C 写入磁盘并与 CaseConfig 中描述的 golden 比对。
// 返回 true 表示匹配或未提供 golden，false 表示不匹配或 IO 错误。
bool write_and_compare(const CaseConfig &cfg, const std::vector<AccType> &C,
                       const std::vector<DataType> &A, const std::vector<DataType> &B);

// 直接使用软件参考实现对 C 进行验证，返回是否通过。
bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                   const std::vector<DataType>& B, int B_rows, int B_cols,
                   const std::vector<AccType>& C);

// 模板：计算 diffs 索引
template<typename T>
std::vector<size_t> compute_diffs(const std::vector<T>& got, const std::vector<T>& expected) {
    std::vector<size_t> diffs;
    size_t n = std::min(got.size(), expected.size());
    for (size_t i = 0; i < n; ++i) {
        if (got[i] != expected[i]) diffs.push_back(i);
    }
    if (got.size() != expected.size()) {
        size_t larger = std::max(got.size(), expected.size());
        for (size_t i = n; i < larger; ++i) diffs.push_back(i);
    }
    return diffs;
}

// 通用矩阵乘法模板：支持任意输入/输出类型与行主存布局。
template<typename OutT, typename InT>
std::vector<OutT> matmul(const InT* A, int M, int K, int lda,
                         const InT* B, int N, int ldb) {
    std::vector<OutT> out;
    if (M <= 0 || N <= 0) return out;
    out.assign(static_cast<size_t>(M) * static_cast<size_t>(N), OutT{});
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            OutT sum = OutT{};
            for (int k = 0; k < K; ++k) {
                sum += static_cast<OutT>(A[static_cast<size_t>(i) * lda + k]) *
                       static_cast<OutT>(B[static_cast<size_t>(k) * ldb + j]);
            }
            out[static_cast<size_t>(i) * N + j] = sum;
        }
    }
    return out;
}

// 便捷重载：接受 std::vector，假设各行紧凑存储。
template<typename OutT, typename InT>
std::vector<OutT> matmul(const std::vector<InT>& A, int M, int K,
                         const std::vector<InT>& B, int N) {
    return matmul<OutT, InT>(A.data(), M, K, K, B.data(), N, N);
}

// 兼容旧调用的包装器：使用 AccType 作为输出类型。
inline std::vector<AccType> compute_reference(const std::vector<DataType>& A, int M, int K,
                                              const std::vector<DataType>& B, int N) {
    return matmul<AccType, DataType>(A, M, K, B, N);
}

// 测试辅助：生成随机 int16 矩阵（行主序）
std::vector<int16_t> generate_random_matrix(int rows, int cols, int min_val = -128, int max_val = 127);

} // namespace util

