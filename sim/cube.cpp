#include "cube.h"
#include "config/config.h"
#include <stdexcept>

// 文件：cube.cpp
// 说明：Cube 实现文件。
// 实现 Cube 的构造与公开接口，负责把 `SystolicArray` 与外部的 `Clock`/`Mem` 连接，
// 并封装配置加载与运行调用。Cube 自身不包含周期级的内部实现。

Cube::Cube(p_clock_t external_clock, p_mem_t external_mem)
            : clock_(external_clock),
            mem_(external_mem) {

    if (!external_clock) {
        throw std::invalid_argument("Cube requires an external clock; provide via SimTop::build_clk and pass it through");
    }
    if (!mem_) {
        throw std::invalid_argument("Cube requires external memory; construct via SimTop::build_mem");
    }

    // construct the internal SystolicArray now that clock/mem are set
    systolic_ = std::make_shared<SystolicArray>(clock_, external_mem);
}


// Run wrapper forwards to the internal SystolicArray implementation.
bool Cube::run(int M, int N, int K,
                          uint32_t a_addr, uint32_t b_addr, uint32_t c_addr) {
    return systolic_->run(M, N, K, a_addr, b_addr, c_addr);
}

