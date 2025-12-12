#ifndef CUBE_TOP_H
#define CUBE_TOP_H

#include <string>

#include "systolic.h"
#include "clock.h"

// cube.h — Cube 顶层封装（中文注释）
// `Cube` 是对 `SystolicArray` 的轻量封装，负责接入时钟与内存，并暴露便捷的
// 算法接口（如 `run`、`verify_result`）。配置按需从文件读取。
// Cube 仅负责模块组合，不实现周期级仿真细节。
// Cube 作为顶层 API，供测试与外部驱动使用。
// Cube 作为顶层封装，负责初始化和持有各个子单元（当前为 SystolicArray）
class Cube {
public:
    explicit Cube(const std::string& config_path,
                  p_clock_t external_clock,
                  p_mem_t external_mem = nullptr);

    // (Configuration is read on-demand via per-key getters; no in-memory struct)

    // (Internal SystolicArray accessors removed — not part of public API.)

    // Provide top-level convenience methods that forward to the internal SystolicArray

    // Run using data already loaded into memory. `a_addr`, `b_addr`, and
    // `c_addr` are the base addresses where A, B and C (accumulators) reside.
    bool run(int M, int N, int K,
                         uint32_t a_addr, uint32_t b_addr, uint32_t c_addr);

    // Note: result verification has been moved to a standalone utility function
    // `verify_result(...)` available in `util/verify.h`.

    // (Clock and memory access are intentionally not exposed here;
    // callers should hold the handles returned from SimTop::build_*)

    // 访问配置
    // Returns config path (file used for configuration)
    const std::string& config_path() const { return config_path_; }

private:
    std::string config_path_;
    p_clock_t clock_;
    p_mem_t mem_;
    p_systolic_array_t systolic_;

    // Load or normalize configuration path and perform any Cube-level
    // configuration/validation. Called from constructor.
    void config(const std::string &path);
};

#endif // CUBE_TOP_H
