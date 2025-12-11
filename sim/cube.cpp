#include "cube.h"
#include "config/config_mgr.h"
#include <stdexcept>

// cube.cpp — Cube 实现（中文注释）
// 本文件实现 Cube 的构造与对外接口，Cube 作为对内部 SystolicArray 的薄封装，
// 负责在顶层将时钟、内存与阵列连接起来，并提供便捷的矩阵乘法与校验接口。

Cube::Cube(const std::string& config_path, p_clock_t external_clock, p_mem_t external_mem)
        : config_path_(config_path),
          clock_(external_clock),
          mem_(external_mem),
          systolic_(config_path, clock_, external_mem) {
    if (!external_clock) {
        throw std::invalid_argument("Cube requires an external clock; provide via SimTop::build_clk and pass it through");
    }
    if (!mem_) {
        throw std::invalid_argument("Cube requires external memory; construct via SimTop::build_mem");
    }
}

bool Cube::matmul(const std::vector<DataType>& A, int A_rows, int A_cols,
                  const std::vector<DataType>& B, int B_rows, int B_cols,
                  std::vector<AccType>& C) {
    return systolic_.matmul(A, A_rows, A_cols, B, B_rows, B_cols, C);
}

