#ifndef AIC_H
#define AIC_H

#include <memory>
#include <string>

#include "cube.h"
#include "clock.h"
#include "mem_if.h"
#include "config/config_mgr.h"

// AIC 封装顶层构建：clock、memory、cube，提供 build_* 接口便于模块化
class AIC {
public:
    explicit AIC(const std::string& config_path = "config.toml");
    // 分步构建接口（build_all 创建 clock/mem 后传给 build_cube）
    p_clock_t build_clk();
    p_mem_t build_mem(const p_clock_t& clk);

    p_cube_t build_cube(const p_clock_t& clk, const p_mem_t& mem);
    
    void build_all();

    p_systolic_array_t array() {
        return cube_ ? p_systolic_array_t(cube_, &cube_->array()) : nullptr;
    }

    // Access to the constructed Cube object (may be nullptr until built)
    p_cube_t get_cube() { return cube_; }

private:
    std::string config_path_;
    p_cube_t cube_;
};

#endif // AIC_H
