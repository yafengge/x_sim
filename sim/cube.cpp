#include "cube.h"
#include "config_loader.h"
#include <stdexcept>

Cube::Cube(const SysConfig& cfg, p_clock_t external_clock, p_mem_t external_mem)
        : config_(cfg),
            clock_(external_clock ? external_clock : p_clock_t(new Clock())),
            mem_(external_mem),
            systolic_(config_, clock_, external_mem) {
        if (!mem_) {
            throw std::invalid_argument("Cube requires external memory; construct via SimTop::build_mem");
        }
}

bool Cube::load_config(const std::string& path, SysConfig& cfg, std::string* err) {
    return load_cube_config(path, cfg, err);
}
