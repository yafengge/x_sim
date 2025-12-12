#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "types.h"
#include <fstream>

namespace util {

struct CaseConfig {
    std::string case_path; // path to the toml itself
    std::string a_path;
    uint32_t a_addr = 0;
    std::string b_path;
    uint32_t b_addr = 0;
    std::string c_golden_path;
    uint32_t c_addr = 0;
    std::string c_out_path;
    // path to the model config (model_cfg.toml) referenced by the case
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

// Binary IO templates were moved to util/utils.h; implementation files
// that need binary IO should include util/utils.h instead of util/bin_io.h.

namespace util {

// write a TOML file for the case. This updates `cfg` to contain the
// absolute paths actually written so callers can reuse them.
bool write_case_toml(CaseConfig &cfg);
// read an existing TOML file into CaseConfig
bool read_case_toml(const std::string &path, CaseConfig &out);

// write a minimal model config file (model_cfg.toml) used by cases
bool write_config_file(const std::string& path, int array_rows, int array_cols);

// Create case binaries and TOML for a given base name. This will create
// `case_dir/base_name_A.bin`, `base_name_B.bin`, `base_name_C_golden.bin` and
// `base_name_C_out.bin`, set addresses and meta in `cfg`, write binaries and
// the TOML. Returns true on success.
// Create a cube-focused case configuration and binaries for a given base name.
// This will create `case_dir/base_name_A.bin`, `base_name_B.bin`,
// `base_name_C_golden.bin` and `base_name_C_out.bin`, set addresses and meta
// in `cfg`, write binaries and the TOML. Returns true on success.
bool create_cube_case_config(const std::string &case_toml, CaseConfig &cfg,
                       const std::string &case_dir, const std::string &base_name,
                       const std::vector<int16_t> &A, const std::vector<int16_t> &B,
                       int M, int K, int N);

} // namespace util
