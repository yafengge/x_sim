#ifndef SIM_TOP_H
#define SIM_TOP_H

#include <memory>

#include "cube.h"
#include "clock.h"
#include "mem_if.h"

// SimTop 封装顶层构建：clock、memory、cube，提供 build_* 接口便于模块化
class SimTop {
public:
    explicit SimTop(const SystolicConfig& cfg);

    // 分步构建接口（使用临时变量，统一在 build_cube 调用）
    std::shared_ptr<Clock> build_clock();
    std::shared_ptr<Mem> build_mem(const std::shared_ptr<Clock>& clk);
    Cube& build_cube();
    void build_all();

    // 访问器
    std::shared_ptr<Clock> clock() { return cube_ ? cube_->clock() : nullptr; }
    std::shared_ptr<const Clock> clock() const { return cube_ ? cube_->clock() : nullptr; }

    std::shared_ptr<Mem> memory() { return cube_ ? cube_->memory() : nullptr; }
    std::shared_ptr<const Mem> memory() const { return cube_ ? cube_->memory() : nullptr; }

    Cube& cube() { return *cube_; }
    const Cube& cube() const { return *cube_; }

    SystolicArray& array() { return cube_->array(); }
    const SystolicArray& array() const { return cube_->array(); }

    const SystolicConfig& config() const { return config_; }

private:
    SystolicConfig config_;
    std::unique_ptr<Cube> cube_;
};

#endif // SIM_TOP_H
