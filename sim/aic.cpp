// aic.cpp — AIC 顶层实现（中文注释）
// 本文件实现 AIC 类的组装逻辑：创建时钟、内存并构建 Cube 实例。
// 注意：此处仅为模块组装，不包含仿真的内部细节，这些在子模块中实现。

#include "aic.h"

#include <iostream>

AIC::AIC(const p_clock_t& clk,
         const p_mem_t& mem)
  : clk_(clk), mem_(mem) {
  // Delayed: cube_ will be constructed in build(config_path)
}

void AIC::build(const std::string& config_path) {
  config_path_ = config_path;
  if (!clk_ || !mem_) {
    std::cerr << "AIC::build: clock or memory not provided" << std::endl;
    return;
  }
  if (!cube_) {
    cube_ = p_cube_t(new Cube(config_path_, clk_, mem_));
  }
}

bool AIC::start(const std::vector<DataType>& A, int A_rows, int A_cols,
                const std::vector<DataType>& B, int B_rows, int B_cols,
                std::vector<AccType>& C) {
  if (!cube_) {
    std::cerr << "AIC::start: cube not constructed; call build(config_path) first" << std::endl;
    return false;
  }
  return cube_->run(A, A_rows, A_cols, B, B_rows, B_cols, C);
}
