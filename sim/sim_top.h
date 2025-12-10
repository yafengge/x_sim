#ifndef SIM_TOP_H
#define SIM_TOP_H

#include <memory>

#include "cube.h"
#include "clock.h"
#include "mem_if.h"
#include "config_loader.h"

// SimTop 封装顶层构建：clock、memory、cube，提供 build_* 接口便于模块化
class SimTop {
public:
    explicit SimTop(const std::string& config_path = "config.toml");
    explicit SimTop(const SystolicConfig& cfg);

    // 分步构建接口（build_all 创建 clock/mem 后传给 build_cube）
    std::shared_ptr<Mem> build_mem(const std::shared_ptr<Clock>& clk);

    std::shared_ptr<Cube> build_cube(const std::shared_ptr<Clock>& clk,
                                     const std::shared_ptr<Mem>& mem);
    void build_all();

    // 访问器
    std::shared_ptr<Clock> clock() { return cube_ ? cube_->clock() : nullptr; }
    std::shared_ptr<const Clock> clock() const { return cube_ ? cube_->clock() : nullptr; }

    std::shared_ptr<Mem> memory() { return cube_ ? cube_->memory() : nullptr; }
    std::shared_ptr<const Mem> memory() const { return cube_ ? cube_->memory() : nullptr; }

    std::shared_ptr<Cube> cube() { return cube_; }
    std::shared_ptr<const Cube> cube() const { return cube_; }

    SystolicArray& array() { return cube_->array(); }
    const SystolicArray& array() const { return cube_->array(); }

private:
    std::string config_path_;
    std::unique_ptr<Cube> cube_;
};

#endif // SIM_TOP_H
