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

// Generic binary read/write helpers (header-only templates)
template<typename T>
inline bool write_bin(const std::string &path, const std::vector<T> &v) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    ofs.write(reinterpret_cast<const char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(T)));
    return ofs.good();
}

template<typename T>
inline bool read_bin(const std::string &path, std::vector<T> &v) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) return false;
    std::streamsize sz = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    if (sz % sizeof(T) != 0) return false;
    size_t count = static_cast<size_t>(sz / sizeof(T));
    v.resize(count);
    ifs.read(reinterpret_cast<char*>(v.data()), sz);
    return ifs.good();
}

// Backwards-compatible wrappers for existing API
inline bool write_bin_int16(const std::string &path, const std::vector<int16_t> &v) { return write_bin<int16_t>(path, v); }
inline bool write_bin_int32(const std::string &path, const std::vector<int32_t> &v) { return write_bin<int32_t>(path, v); }
inline bool read_bin_int16(const std::string &path, std::vector<int16_t> &v) { return read_bin<int16_t>(path, v); }
inline bool read_bin_int32(const std::string &path, std::vector<int32_t> &v) { return read_bin<int32_t>(path, v); }
// Flexible read: try given path, then search upward from cwd, then PROJECT_SRC_DIR
bool read_bin_int16_flexible(const std::string &path, std::vector<DataType> &v);

// convenience wrapper for int32 write used by AIC
inline bool write_bin_int32_from_acc(const std::string &path, const std::vector<AccType> &v) { return write_bin<AccType>(path, v); }

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
