#ifndef VERIFY_UTIL_H
#define VERIFY_UTIL_H

#include <vector>
#include <iostream>
#include <cmath>
#include "types.h"

namespace xsim {
namespace util {

// 独立的结果校验工具函数，提供软件参考实现的矩阵乘法校验。
// 这个函数从原来的类方法中抽离出来，供测试和工具直接调用。
bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                   const std::vector<DataType>& B, int B_rows, int B_cols,
                   const std::vector<AccType>& C);

} // namespace util
} // namespace xsim

#endif // VERIFY_UTIL_H
