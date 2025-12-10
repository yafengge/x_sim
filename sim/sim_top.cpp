#include "sim_top.h"

#include <iostream>

SimTop::SimTop(const std::string& config_path)
  : config_path_(config_path) {
  build_all();
}

SimTop::SimTop(const SystolicConfig& cfg)
  : config_path_("config.toml") {
  build_all(cfg);
}

Cube& SimTop::build_cube(const std::shared_ptr<Clock>& clk,
                         const std::shared_ptr<Mem>& mem,
                         const SystolicConfig& cfg) {
  if (!cube_) {
    cube_ = std::make_unique<Cube>(cfg, clk, mem);
  }
  return *cube_;
}

void SimTop::build_all() {
  SystolicConfig cfg;
  std::string err;
  Cube::load_config(config_path_, cfg, &err);
  std::string mem_err;
  Mem::load_config(config_path_, cfg, &mem_err);
  if (!err.empty()) {
    std::cerr << "Cube config load warning: " << err << "\n";
  }
  if (!mem_err.empty()) {
    std::cerr << "Memory config load warning: " << mem_err << "\n";
  }
  config_ = cfg;

  auto clk = std::make_shared<Clock>();
  int max_outstanding = cfg.max_outstanding > 0 ? cfg.max_outstanding : 0;
  int issue_bw = cfg.bandwidth;
  int complete_bw = cfg.bandwidth;
  auto mem = std::make_shared<Mem>(64, cfg.memory_latency, issue_bw, complete_bw, max_outstanding, clk);
  build_cube(clk, mem, config_);
}

void SimTop::build_all(const SystolicConfig& cfg) {
  config_ = cfg;

  auto clk = std::make_shared<Clock>();
  int max_outstanding = cfg.max_outstanding > 0 ? cfg.max_outstanding : 0;
  int issue_bw = cfg.bandwidth;
  int complete_bw = cfg.bandwidth;
  auto mem = std::make_shared<Mem>(64, cfg.memory_latency, issue_bw, complete_bw, max_outstanding, clk);
  build_cube(clk, mem, config_);
}
