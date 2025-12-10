#ifndef SIM_TOP_H
#define SIM_TOP_H

#include <memory>

#include "cube.h"
#include "clock.h"

// SimTop 封装外部时钟与 Cube，供测试或上层驱动复用
class SimTop {
public:
    explicit SimTop(const SystolicConfig& cfg);

    std::shared_ptr<Clock> clock() { return clock_; }
    std::shared_ptr<const Clock> clock() const { return clock_; }

    Cube& cube() { return *cube_; }
    const Cube& cube() const { return *cube_; }

    SystolicArray& array() { return cube_->array(); }
    const SystolicArray& array() const { return cube_->array(); }

    const SystolicConfig& config() const { return cube_->config(); }

private:
    std::shared_ptr<Clock> clock_;
    std::unique_ptr<Cube> cube_;
};

#endif // SIM_TOP_H
