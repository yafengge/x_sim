#ifndef CUBE_TOP_H
#define CUBE_TOP_H

#include <string>

#include "systolic.h"
#include "clock.h"

// cube.h — Cube 顶层封装（中文注释）
// `Cube` 是对 `SystolicArray` 的轻量封装，负责接入时钟与内存，并暴露便捷的
// 算法接口（如 `matmul`、`verify_result`）。配置按需从文件读取。
// Cube 仅负责模块组合，不实现周期级仿真细节。
// Cube 作为顶层 API，供测试与外部驱动使用。
// Cube 作为顶层封装，负责初始化和持有各个子单元（当前为 SystolicArray）
class Cube {
public:
    explicit Cube(const std::string& config_path,
                  p_clock_t external_clock,
                  p_mem_t external_mem = nullptr);

    // (Configuration is read on-demand via per-key getters; no in-memory struct)

    // 访问内部的脉动阵列实例
    SystolicArray& array() { return *systolic_; }
    const SystolicArray& array() const { return *systolic_; }

    // Provide top-level convenience methods that forward to the internal SystolicArray
    bool matmul(const std::vector<DataType>& A, int A_rows, int A_cols,
                const std::vector<DataType>& B, int B_rows, int B_cols,
                std::vector<AccType>& C);

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
