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

// Binary IO templates are provided in util/bin_io.h
#include "util/bin_io.h"
// Flexible read: try given path, then search upward from cwd, then PROJECT_SRC_DIR
bool read_bin_int16_flexible(const std::string &path, std::vector<DataType> &v);

// convenience wrapper for int32 write used by AIC
inline bool write_bin_int32_from_acc(const std::string &path, const std::vector<AccType> &v) { return util::write_bin<AccType>(path, v); }

// write a TOML file for the case. This updates `cfg` to contain the
// absolute paths actually written so callers can reuse them.
bool write_case_toml(CaseConfig &cfg);
// read an existing TOML file into CaseConfig
bool read_case_toml(const std::string &path, CaseConfig &out);

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
