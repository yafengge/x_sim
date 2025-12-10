#include "sim_top.h"

SimTop::SimTop(const SystolicConfig& cfg)
  : config_(cfg) {
  build_all();
}

std::shared_ptr<Clock> SimTop::build_clock() {
  return std::make_shared<Clock>();
}

std::shared_ptr<Mem> SimTop::build_mem(const std::shared_ptr<Clock>& clk) {
  int max_outstanding = config_.max_outstanding > 0 ? config_.max_outstanding : 0;
  int issue_bw = config_.bandwidth;
  int complete_bw = config_.bandwidth;
  return std::make_shared<Mem>(64, config_.memory_latency, issue_bw, complete_bw, max_outstanding, clk);
}

Cube& SimTop::build_cube() {
  if (!cube_) {
    auto clk = build_clock();
    auto mem = build_mem(clk);
    cube_ = std::make_unique<Cube>(config_, clk, mem);
  }
  return *cube_;
}

void SimTop::build_all() {
  build_cube();
}
