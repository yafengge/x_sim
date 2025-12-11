// aic.cpp — AIC 顶层实现（中文注释）
// 本文件实现 AIC 类的组装逻辑：创建时钟、内存并构建 Cube 实例。
// 注意：此处仅为模块组装，不包含仿真的内部细节，这些在子模块中实现。

#include "aic.h"

#include <iostream>

AIC::AIC(const std::string& config_path,
         const p_clock_t& clk,
         const p_mem_t& mem)
  : config_path_(config_path) {
  // construct Cube with provided clock and memory
  cube_ = p_cube_t(new Cube(config_path_, clk, mem));
}

bool AIC::start(const std::vector<DataType>& A, int A_rows, int A_cols,
                const std::vector<DataType>& B, int B_rows, int B_cols,
                std::vector<AccType>& C) {
  if (!cube_) {
    std::cerr << "AIC::start: cube not constructed/attached" << std::endl;
    return false;
  }
  return cube_->run(A, A_rows, A_cols, B, B_rows, B_cols, C);
}
