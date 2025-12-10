#ifndef CUBE_TOP_H
#define CUBE_TOP_H

#include <string>

#include "systolic.h"
#include "clock.h"

// Cube 作为顶层封装，负责初始化和持有各个子单元（当前为 SystolicArray）
class Cube {
public:
    explicit Cube(const SystolicConfig& cfg,
                  std::shared_ptr<Clock> external_clock = nullptr,
                  std::shared_ptr<Mem> external_mem = nullptr);

    // 加载 cube 配置段
    static bool load_config(const std::string& path, SystolicConfig& cfg, std::string* err = nullptr);

    // 外部可以驱动同一个时钟
    std::shared_ptr<Clock> clock() { return clock_; }
    std::shared_ptr<const Clock> clock() const { return clock_; }

    // 访问内部的脉动阵列实例
    SystolicArray& array() { return systolic_; }
    const SystolicArray& array() const { return systolic_; }

    // 访问内存模型
    std::shared_ptr<Mem> memory() { return mem_; }
    std::shared_ptr<const Mem> memory() const { return mem_; }

    // 访问配置
    const SystolicConfig& config() const { return config_; }

private:
    SystolicConfig config_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<Mem> mem_;
    SystolicArray systolic_;
};

#endif // CUBE_TOP_H
