#ifndef SIM_TOP_H
#define SIM_TOP_H

#include <memory>

#include "cube.h"
#include "clock.h"
#include "mem_if.h"
#include "config_mgr.h"

// SimTop 封装顶层构建：clock、memory、cube，提供 build_* 接口便于模块化
class SimTop {
public:
    explicit SimTop(const std::string& config_path = "config.toml");
    // Note: struct-based constructor removed; use file-based config path only.

    // 分步构建接口（build_all 创建 clock/mem 后传给 build_cube）
    p_clock_t build_clk();
    p_mem_t build_mem(const p_clock_t& clk);

    p_cube_t build_cube(const p_clock_t& clk, const p_mem_t& mem);
    
    void build_all();

    // 访问器
    p_clock_t clock() { return cube_ ? cube_->clock() : nullptr; }

    p_mem_t memory() { return cube_ ? cube_->memory() : nullptr; }

    p_cube_t cube() { return cube_; }

    p_systolic_array_t array() {
        return cube_ ? p_systolic_array_t(cube_, &cube_->array()) : nullptr;
    }

private:
    std::string config_path_;
    p_cube_t cube_;
};

#endif // SIM_TOP_H
