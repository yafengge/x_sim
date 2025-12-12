#ifndef CUBE_TOP_H
#define CUBE_TOP_H

#include <string>

#include "systolic.h"
#include "clock.h"

// 文件：cube.h
// 说明：Cube 顶层封装。
// `Cube` 是对 `SystolicArray` 的薄包装，负责接入外部时钟与内存，读取/规范化配置，
// 并提供对外的 `run` 等便捷接口供测试和工具调用。周期级实现仍留在子模块中。
class Cube {
public:
    explicit Cube(p_clock_t external_clock,
                  p_mem_t external_mem = nullptr);

    // Run using data already loaded into memory. `a_addr`, `b_addr`, and
    // `c_addr` are the base addresses where A, B and C (accumulators) reside.
    bool run(int M, int N, int K,
                         uint32_t a_addr, uint32_t b_addr, uint32_t c_addr);

private:
    p_clock_t clock_;
    p_mem_t mem_;
    p_systolic_array_t systolic_;

    void config(const std::string &path);
};

#endif // CUBE_TOP_H
