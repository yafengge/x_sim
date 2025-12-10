#include "cube.h"
#include "config/config_mgr.h"
#include <stdexcept>

Cube::Cube(const std::string& config_path, p_clock_t external_clock, p_mem_t external_mem)
        : config_path_(config_path),
          clock_(external_clock),
          mem_(external_mem),
          systolic_(config_path, clock_, external_mem) {
    if (!external_clock) {
        throw std::invalid_argument("Cube requires an external clock; provide via SimTop::build_clk and pass it through");
    }
    if (!mem_) {
        throw std::invalid_argument("Cube requires external memory; construct via SimTop::build_mem");
    }
}
