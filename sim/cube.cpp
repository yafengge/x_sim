#include "cube.h"
#include "config_loader.h"

Cube::Cube(const SysConfig& cfg, p_clock_t external_clock, p_mem_t external_mem)
        : config_(cfg),
            clock_(external_clock ? external_clock : p_clock_t(new Clock())),
            mem_(external_mem),
            systolic_(config_, clock_, external_mem) {
        // 若未传入外部内存，获取 SystolicArray 创建的内存实例以便外部访问
        if (!mem_) {
            mem_ = systolic_.get_memory();
        }
}

bool Cube::load_config(const std::string& path, SysConfig& cfg, std::string* err) {
    return load_cube_config(path, cfg, err);
}
