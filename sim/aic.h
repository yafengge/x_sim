// AIC 顶层封装（中文注释）
// 本文件提供对仿真顶层的封装：构造时钟、内存与 Cube，并暴露分步构建接口。
// 该类仅做组装与便捷调用，不包含周期级实现逻辑，详细实现放在子模块中。
#ifndef AIC_H
#define AIC_H

#include <memory>
#include <string>

#include "cube.h"
#include "clock.h"
#include "mem_if.h"
#include "config/config_mgr.h"
// utilities for per-case TOML and binary I/O
#include "util/case_io.h"

// AIC 封装顶层构建：clock、memory、cube，提供 build_* 接口便于模块化
class AIC {
public:
    // Construct AIC with clock and memory; call `build(cfg)` to configure and
    // construct the internal `Cube` before calling `start()`.
    explicit AIC(const p_clock_t& clk,
                 const p_mem_t& mem);

    // Configure and construct the Cube using a per-case TOML. The case TOML
    // should reference the platform `model_cfg.toml`. If `force` is true,
    // reinitialize the Cube even if the config matches.
    void build(const std::string& case_toml_path, bool force = false);

    // Access to the constructed Cube object has been removed; use AIC::start
    // to perform runs via the AIC interface.

    // Start a matrix multiply run via the attached Cube. Returns false if Cube not attached.
    bool start(const std::vector<DataType>& A, int A_rows, int A_cols,
               const std::vector<DataType>& B, int B_rows, int B_cols,
               std::vector<AccType>& C);

    // Start a run using the previously provided case TOML passed to `build`.
    // The A/B files will be loaded by AIC (from disk) and the resulting C
    // will be written to the configured output path; if a golden exists it
    // will be compared and differences printed.
    bool start();

private:
    // configuration path (set via build)
    std::string config_path_;
    // retained handles for constructing Cube during build
    p_clock_t clk_;
    p_mem_t mem_;
    p_cube_t cube_;
    // Helper methods
    bool read_bins_from_cfg(const util::CaseConfig &cfg, std::vector<DataType> &A, std::vector<DataType> &B);
    std::string resolve_path(const std::string &path) const;
    void preload_into_mem(const util::CaseConfig &cfg, const std::vector<DataType> &A, const std::vector<DataType> &B);
    bool write_and_compare(const util::CaseConfig &cfg, const std::vector<AccType> &C,
                           const std::vector<DataType> &A, const std::vector<DataType> &B);
    // stored case configuration (set by build)
    util::CaseConfig case_cfg_;
};

#endif // AIC_H
