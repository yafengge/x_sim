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
    // construct the internal `Cube` before calling `start()`.
    explicit AIC(const p_clock_t& clk,
                 const p_mem_t& mem);

    void build(const std::string& case_toml_path, bool force = false);

    bool start();

private:
    
    p_clock_t clk_;
    p_mem_t mem_;
    p_cube_t cube_;
    // Helper methods
    void preload_into_mem(const util::CaseConfig &cfg, const std::vector<DataType> &A, const std::vector<DataType> &B);
    // stored case configuration (set by build)
    util::CaseConfig case_cfg_;
    std::string config_path_;
};

#endif // AIC_H
