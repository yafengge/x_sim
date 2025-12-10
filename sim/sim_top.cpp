#include "sim_top.h"

#include <iostream>

SimTop::SimTop(const std::string& config_path)
  : config_path_(config_path) {
  build_all();
}

// struct-based constructor removed.

p_cube_t SimTop::build_cube(const p_clock_t& clk,
                            const p_mem_t& mem) {
  if (!cube_) {
    // Cube will read configuration on demand from the provided config path
    cube_ = p_cube_t(new Cube(config_path_, clk, mem));
  }
  return cube_;
}

p_clock_t SimTop::build_clk() {
  return p_clock_t(new Clock());
}

p_mem_t SimTop::build_mem(const p_clock_t& clk) {
  // Read memory configuration values on demand (no caching)
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

void SimTop::build_all() {
  auto clk = build_clk();
  auto mem = build_mem(clk);
  build_cube(clk, mem);
}
