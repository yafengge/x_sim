#include "sim_top.h"

#include <iostream>

SimTop::SimTop(const std::string& config_path)
  : config_path_(config_path), has_override_config_(false) {
  build_all();
}

SimTop::SimTop(const std::string& config_path, const SysConfig& override_cfg)
  : config_path_(config_path), has_override_config_(true), override_cfg_(override_cfg) {
  build_all();
}

p_cube_t SimTop::build_cube(const p_clock_t& clk,
                            const p_mem_t& mem) {
  if (!cube_) {
    SysConfig cfg;
    std::string err;
    if (has_override_config_) {
      cfg = override_cfg_;
    } else {
      Cube::load_config(config_path_, cfg, &err);
      if (!err.empty()) {
        std::cerr << "Cube config load warning: " << err << "\n";
      }
    }
    cube_ = p_cube_t(new Cube(cfg, clk, mem));
  }
  return cube_;
}

p_clock_t SimTop::build_clk() {
  return p_clock_t(new Clock());
}

p_mem_t SimTop::build_mem(const p_clock_t& clk) {
  SysConfig cfg;
  std::string err;
  if (has_override_config_) {
    cfg = override_cfg_;
  } else {
    Mem::load_config(config_path_, cfg, &err);
    if (!err.empty()) {
      std::cerr << "Memory config load warning: " << err << "\n";
    }
  }

  int max_outstanding = cfg.max_outstanding > 0 ? cfg.max_outstanding : 0;
  int issue_bw = cfg.bandwidth;
  int complete_bw = cfg.bandwidth;
  return p_mem_t(new Mem(64, cfg.memory_latency, issue_bw, complete_bw, max_outstanding, clk));
}

void SimTop::build_all() {
  auto clk = build_clk();
  auto mem = build_mem(clk);
  build_cube(clk, mem);
}
