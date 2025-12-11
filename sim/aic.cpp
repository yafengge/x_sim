// aic.cpp — AIC 顶层实现（中文注释）
// 本文件实现 AIC 类的组装逻辑：创建时钟、内存并构建 Cube 实例。
// 注意：此处仅为模块组装，不包含仿真的内部细节，这些在子模块中实现。

#include "aic.h"

#include <iostream>

AIC::AIC(const std::string& config_path)
  : config_path_(config_path) {
  build_all();
}

p_cube_t AIC::build_cube(const p_clock_t& clk,
                            const p_mem_t& mem) {
  if (!cube_) {
    // Cube 将按需从配置文件读取参数
    cube_ = p_cube_t(new Cube(config_path_, clk, mem));
  }
  return cube_;
}



p_mem_t AIC::build_mem(const p_clock_t& clk) {
  // 按需读取内存相关配置（不缓存）
  int mem_latency = 10;
  int bw = 4;
  int max_outstanding = 0;
  std::string tmp;
  if (!get_int_key(config_path_, "memory.memory_latency", mem_latency)) mem_latency = 10;
  if (!get_int_key(config_path_, "memory.bandwidth", bw)) bw = 4;
  if (!get_int_key(config_path_, "memory.max_outstanding", max_outstanding)) max_outstanding = 0;
  int issue_bw = bw;
  int complete_bw = bw;
  return p_mem_t(new Mem(64, mem_latency, issue_bw, complete_bw, max_outstanding, clk));
}

void AIC::build_all() {
  auto clk = std::make_shared<Clock>();
  auto mem = build_mem(clk);
  build_cube(clk, mem);
}
