#ifndef VERIFY_UTIL_H
#define VERIFY_UTIL_H

#include <vector>
#include <iostream>
#include <cmath>
#include "types.h"
#include "case_io.h"
#include <string>

namespace util {

// 独立的结果校验工具函数，提供软件参考实现的矩阵乘法校验。
// 这个函数从原来的类方法中抽离出来，供测试和工具直接调用。
bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                   const std::vector<DataType>& B, int B_rows, int B_cols,
                   const std::vector<AccType>& C);

// Resolve a possibly-relative path using PROJECT_SRC_DIR when needed
std::string resolve_path(const std::string &path);

// Print simple diffs between two int32 vectors (used by write_and_compare)
void print_diffs(const std::vector<int32_t>& a, const std::vector<int32_t>& b);

// Write C (accumulators) to disk and compare with golden described in CaseConfig.
// Returns true if results match or no golden provided; false if mismatch or IO error.
bool write_and_compare(const CaseConfig &cfg, const std::vector<AccType> &C,
                       const std::vector<DataType> &A, const std::vector<DataType> &B);

} // namespace util

#endif // VERIFY_UTIL_H
