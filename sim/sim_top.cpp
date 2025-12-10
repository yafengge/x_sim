#include "sim_top.h"

#include <iostream>

SimTop::SimTop(const std::string& config_path)
  : config_path_(config_path) {
  build_all();
}

SimTop::SimTop(const SysConfig& cfg)
  : config_path_("config.toml") {
  (void)cfg;
  build_all();
}

p_cube_t SimTop::build_cube(const p_clock_t& clk,
                            const p_mem_t& mem) {
  if (!cube_) {
    SysConfig cfg;
    std::string err;
    Cube::load_config(config_path_, cfg, &err);
    if (!err.empty()) {
      std::cerr << "Cube config load warning: " << err << "\n";
    }
    cube_ = std::make_shared<Cube>(cfg, clk, mem);
  }
  return cube_;
}

p_mem_t SimTop::build_mem(const p_clock_t& clk) {
  SysConfig cfg;
  std::string err;
  Mem::load_config(config_path_, cfg, &err);
  if (!err.empty()) {
    std::cerr << "Memory config load warning: " << err << "\n";
  }

  int max_outstanding = cfg.max_outstanding > 0 ? cfg.max_outstanding : 0;
  int issue_bw = cfg.bandwidth;
  int complete_bw = cfg.bandwidth;
  return std::make_shared<Mem>(64, cfg.memory_latency, issue_bw, complete_bw, max_outstanding, clk);
}

void SimTop::build_all() {
  auto clk = std::make_shared<Clock>();
  auto mem = build_mem(clk);
  build_cube(clk, mem);
}
