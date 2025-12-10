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
                         const std::shared_ptr<Mem>& mem) {
  if (!cube_) {
    SystolicConfig cfg;
    std::string err;
    Cube::load_config(config_path_, cfg, &err);
    if (!err.empty()) {
      std::cerr << "Cube config load warning: " << err << "\n";
    }
    cube_ = std::make_unique<Cube>(cfg, clk, mem);
  }
  return *cube_;
}

std::shared_ptr<Mem> SimTop::build_mem(const std::shared_ptr<Clock>& clk) {
  SystolicConfig cfg;
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
