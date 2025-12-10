#ifndef CUBE_TOP_H
#define CUBE_TOP_H

#include <string>

#include "systolic.h"
#include "clock.h"

// Cube 作为顶层封装，负责初始化和持有各个子单元（当前为 SystolicArray）
class Cube {
public:
    explicit Cube(const std::string& config_path,
                  p_clock_t external_clock,
                  p_mem_t external_mem = nullptr);

    // (Configuration is read on-demand via per-key getters; no in-memory struct)

    // 外部可以驱动同一个时钟
    p_clock_t clock() { return clock_; }
    p_clock_t clock() const { return clock_; }

    // 访问内部的脉动阵列实例
    SystolicArray& array() { return systolic_; }
    const SystolicArray& array() const { return systolic_; }

    // 访问内存模型
    p_mem_t memory() { return mem_; }
    p_mem_t memory() const { return mem_; }

    // 访问配置
    // Returns config path (file used for configuration)
    const std::string& config_path() const { return config_path_; }

private:
    std::string config_path_;
    p_clock_t clock_;
    p_mem_t mem_;
    SystolicArray systolic_;
};

#endif // CUBE_TOP_H
