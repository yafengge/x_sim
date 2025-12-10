#include "sim_top.h"

SimTop::SimTop(const SystolicConfig& cfg)
  : config_(cfg) {
  build_all();
}

Cube& SimTop::build_cube(const std::shared_ptr<Clock>& clk,
                         const std::shared_ptr<Mem>& mem) {
  if (!cube_) {
    cube_ = std::make_unique<Cube>(config_, clk, mem);
  }
  return *cube_;
}

void SimTop::build_all() {
  auto clk = std::make_shared<Clock>();
  int max_outstanding = config_.max_outstanding > 0 ? config_.max_outstanding : 0;
  int issue_bw = config_.bandwidth;
  int complete_bw = config_.bandwidth;
  auto mem = std::make_shared<Mem>(64, config_.memory_latency, issue_bw, complete_bw, max_outstanding, clk);
  build_cube(clk, mem);
}
