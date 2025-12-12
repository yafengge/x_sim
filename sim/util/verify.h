// Verification utilities header
#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <iostream>
#include "types.h"
#include "util/case_io.h"

namespace util {

// Print simple diffs between two int32 vectors (used by write_and_compare)
// Forward declaration of templated compute_diffs so print_diffs may call it.
template<typename T>
std::vector<size_t> compute_diffs(const std::vector<T>& got, const std::vector<T>& expected);

// Generic diffs printer (prints up to a limited number of diffs).
// If `indices` is provided, only those indices are printed; otherwise
// the function will compute differences up to `max_print` entries.
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
            std::cerr << "diff at " << idx << ": expected=" << e << " got=" << g << "\n";
            ++diffs_printed;
        }
    } else {
        for (size_t i = 0; i < n && diffs_printed < max_print; ++i) {
            if (got[i] != expected[i]) {
                std::cerr << "diff at " << i << ": expected=" << expected[i] << " got=" << got[i] << "\n";
                ++diffs_printed;
            }
        }
    }
    if (got.size() != expected.size()) {
        std::cerr << "size mismatch: got=" << got.size() << " golden=" << expected.size() << "\n";
    }
    // print total differences count (compute by calling compute_diffs)
    auto total_diffs = compute_diffs<T>(got, expected);
    std::cerr << "total differences: " << total_diffs.size() << "\n";
}

// Write C (accumulators) to disk and compare with golden described in CaseConfig.
// Returns true if results match or no golden provided; false if mismatch or IO error.
bool write_and_compare(const CaseConfig &cfg, const std::vector<AccType> &C,
                       const std::vector<DataType> &A, const std::vector<DataType> &B);

// Software verification helper: compute reference and compare with C
bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                   const std::vector<DataType>& B, int B_rows, int B_cols,
                   const std::vector<AccType>& C);

// Compute diffs between two vectors of arbitrary integral type; returns indices that differ.
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

// Compute software reference matrix multiply: returns MxN AccType vector
// Generic matrix multiply supporting arbitrary input and output types and
// leading dimensions (lda, ldb). A is MxK, B is KxN, result is MxN with
// row-major layout and stride parameters.
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

// Convenience overloads for std::vector inputs assuming tightly-packed rows
template<typename OutT, typename InT>
std::vector<OutT> matmul(const std::vector<InT>& A, int M, int K,
                         const std::vector<InT>& B, int N) {
    return matmul<OutT, InT>(A.data(), M, K, K, B.data(), N, N);
}

// Backwards-compatible wrapper used by existing callers
inline std::vector<AccType> compute_reference(const std::vector<DataType>& A, int M, int K,
                                              const std::vector<DataType>& B, int N) {
    return matmul<AccType, DataType>(A, M, K, B, N);
}

// Test helper: generate a random int16 matrix (row-major)
std::vector<int16_t> generate_random_matrix(int rows, int cols, int min_val = -128, int max_val = 127);

// (report generation removed) emit_report was removed — use compute_diffs
// and print_diffs for simple diagnostics instead.

} // namespace util

