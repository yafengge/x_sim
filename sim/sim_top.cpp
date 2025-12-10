#include "sim_top.h"

SimTop::SimTop(const SystolicConfig& cfg)
    : clock_(std::make_shared<Clock>()),
      cube_(std::make_unique<Cube>(cfg, clock_)) {}
