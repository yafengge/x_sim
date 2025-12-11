// aic.cpp — AIC 顶层实现（中文注释）
// 本文件实现 AIC 类的组装逻辑：创建时钟、内存并构建 Cube 实例。
// 注意：此处仅为模块组装，不包含仿真的内部细节，这些在子模块中实现。

#include "aic.h"

#include <iostream>

AIC::AIC(const std::string& config_path)
  : config_path_(config_path) {
  build_all();
}

p_cube_t AIC::attach(const p_clock_t& clk,
                            const p_mem_t& mem) {
  if (!cube_) {
    // Cube 将按需从配置文件读取参数
    cube_ = p_cube_t(new Cube(config_path_, clk, mem));
  }
  return cube_;
}



p_mem_t AIC::build_mem(const p_clock_t& clk) {
  // Let Mem read its own configuration from the configured path. Pass the
  // AIC's config path so tests / users can control which config is used.
  return p_mem_t(new Mem(clk, config_path_));
}

void AIC::build_all() {
  auto clk = std::make_shared<Clock>();
  auto mem = build_mem(clk);
  attach(clk, mem);
}
