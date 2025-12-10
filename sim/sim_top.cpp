#include "sim_top.h"

SimTop::SimTop(const SystolicConfig& cfg)
  : config_(cfg) {
  build_all();
}

Clock& SimTop::build_clock() {
  if (!clock_) {
    clock_ = std::make_shared<Clock>();
  }
  return *clock_;
}

Mem& SimTop::build_mem() {
  if (!clock_) build_clock();
  if (!mem_) {
    int max_outstanding = config_.max_outstanding > 0 ? config_.max_outstanding : 0;
    int issue_bw = config_.bandwidth;
    int complete_bw = config_.bandwidth;
    mem_ = std::make_shared<Mem>(64, config_.memory_latency, issue_bw, complete_bw, max_outstanding, clock_);
  }
  return *mem_;
}

Cube& SimTop::build_cube() {
  if (!clock_) build_clock();
  if (!mem_) build_mem();
  if (!cube_) {
    cube_ = std::make_unique<Cube>(config_, clock_, mem_);
  }
  return *cube_;
}

void SimTop::build_all() {
  build_clock();
  build_mem();
  build_cube();
}
