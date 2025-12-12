#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "types.h"
#include <fstream>

// 文件：util/case_io.h
// 说明：提供用于生成/读取单个测试 case 的 TOML 与二进制文件的辅助接口。
//       - `CaseConfig` 保存 case 的路径、二进制文件位置与矩阵元数据。
//       - 二进制读写模板位于 `util/utils.h`（header-only）。

namespace util {

struct CaseConfig {
    std::string case_path; // case TOML 文件路径
    std::string a_path;    // A 矩阵二进制路径
    uint32_t a_addr = 0;   // A 的内存地址偏移
    std::string b_path;    // B 矩阵二进制路径
    uint32_t b_addr = 0;   // B 的内存地址偏移
    std::string c_golden_path; // 参考（golden）C 矩阵文件
    uint32_t c_addr = 0;   // C 的内存地址偏移
    std::string c_out_path; // 运行时输出文件（可选）
    // 引用的 model_cfg.toml 路径（相对或绝对）
    std::string model_cfg_path;
    int M = 0;
    int K = 0;
    int N = 0;
    std::string endian = "little";
    std::string a_type = "int16";
    std::string b_type = "int16";
    std::string c_type = "int32";
};

} // namespace util

// 注意：需要二进制 IO 的实现文件请包含 `util/utils.h`，不要包含已弃用的 bin_io.h。

namespace util {

// 将 `cfg` 写为 TOML 文件，写入时会把二进制路径转换为绝对路径并更新 `cfg`。
bool write_case_toml(CaseConfig &cfg);
// 从 TOML 读取到 CaseConfig；相对路径按 TOML 所在目录解析。
bool read_case_toml(const std::string &path, CaseConfig &out);

// 写入最小的 model_cfg.toml，用于描述 PE 阵列尺寸。
bool write_config_file(const std::string& path, int array_rows, int array_cols);

// 依据给定 base_name 在 case_dir 下创建 A/B/C_golden/C_out 的二进制文件并生成 TOML。
// 成功返回 true。
bool create_cube_case_config(const std::string &case_toml, CaseConfig &cfg,
                       const std::string &case_dir, const std::string &base_name,
                       const std::vector<int16_t> &A, const std::vector<int16_t> &B,
                       int M, int K, int N);

} // namespace util
