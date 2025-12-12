#include "util/case_io.h"
#include "config/mini_toml.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <filesystem>

namespace util {

static bool write_file(const std::string &path, const char *data, size_t bytes) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    ofs.write(data, bytes);
    return ofs.good();
}

bool write_bin_int16(const std::string &path, const std::vector<int16_t> &v) {
    return write_file(path, reinterpret_cast<const char*>(v.data()), v.size() * sizeof(int16_t));
}

bool write_bin_int32(const std::string &path, const std::vector<int32_t> &v) {
    return write_file(path, reinterpret_cast<const char*>(v.data()), v.size() * sizeof(int32_t));
}

bool write_bin_int32_from_acc(const std::string &path, const std::vector<AccType> &v) {
    // AccType is int32_t in types.h
    return write_file(path, reinterpret_cast<const char*>(v.data()), v.size() * sizeof(AccType));
}

bool read_bin_int16(const std::string &path, std::vector<int16_t> &v) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) return false;
    std::streamsize sz = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    if (sz % sizeof(int16_t) != 0) return false;
    size_t count = static_cast<size_t>(sz) / sizeof(int16_t);
    v.resize(count);
    ifs.read(reinterpret_cast<char*>(v.data()), sz);
    return ifs.good();
}

bool read_bin_int16_flexible(const std::string &path, std::vector<DataType> &v) {
    // try direct read first
    std::vector<int16_t> tmp;
    if (read_bin_int16(path, tmp)) { v.assign(tmp.begin(), tmp.end()); return true; }
    std::filesystem::path p(path);
    if (p.is_relative()) {
        std::filesystem::path cwd = std::filesystem::current_path();
        for (int depth = 0; depth < 5; ++depth) {
            std::string candidate = (cwd / path).string();
            if (read_bin_int16(candidate, tmp)) { v.assign(tmp.begin(), tmp.end()); return true; }
            if (cwd.has_parent_path()) cwd = cwd.parent_path(); else break;
        }
        std::string alt = std::string(PROJECT_SRC_DIR) + std::string("/") + path;
        if (read_bin_int16(alt, tmp)) { v.assign(tmp.begin(), tmp.end()); return true; }
    }
    return false;
}

bool read_bin_int32(const std::string &path, std::vector<int32_t> &v) {
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs) return false;
    std::streamsize sz = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    if (sz % sizeof(int32_t) != 0) return false;
    size_t count = static_cast<size_t>(sz) / sizeof(int32_t);
    v.resize(count);
    ifs.read(reinterpret_cast<char*>(v.data()), sz);
    return ifs.good();
}

// Very small TOML writer for our case format.
bool write_case_toml(CaseConfig &cfg) {
    // Ensure the directory for the TOML exists.
    try {
        std::filesystem::path case_p(cfg.case_path);
        if (case_p.has_parent_path()) std::filesystem::create_directories(case_p.parent_path());
    } catch (...) {
        // ignore directory creation errors; writing will fail below if necessary
    }

    std::ofstream ofs(cfg.case_path);
    if (!ofs) return false;

    // Write absolute paths for the binaries so the TOML is not dependent on
    // the current working directory when consumed later.
    std::filesystem::path a_p = std::filesystem::absolute(cfg.a_path);
    std::filesystem::path b_p = std::filesystem::absolute(cfg.b_path);
    std::filesystem::path cgold_p = std::filesystem::absolute(cfg.c_golden_path);
    std::filesystem::path cout_p = std::filesystem::absolute(cfg.c_out_path);

    // Update cfg to point to the absolute paths we wrote so callers can use
    // them directly without guessing about working directories.
    cfg.a_path = a_p.string();
    cfg.b_path = b_p.string();
    cfg.c_golden_path = cgold_p.string();
    cfg.c_out_path = cout_p.string();
    try {
        cfg.case_path = std::filesystem::absolute(cfg.case_path).string();
    } catch(...) {
        // ignore
    }
    ofs << "# case toml generated\n";
    ofs << "[input.A]\n";
    ofs << "path = \"" << a_p.string() << "\"\n";
    ofs << "addr = " << cfg.a_addr << "\n";
    ofs << "type = \"" << cfg.a_type << "\"\n";
    ofs << "[input.B]\n";
    ofs << "path = \"" << b_p.string() << "\"\n";
    ofs << "addr = " << cfg.b_addr << "\n";
    ofs << "type = \"" << cfg.b_type << "\"\n";
    ofs << "[output.C]\n";
    ofs << "golden = \"" << cgold_p.string() << "\"\n";
    ofs << "out = \"" << cout_p.string() << "\"\n";
    ofs << "addr = " << cfg.c_addr << "\n";
    ofs << "type = \"" << cfg.c_type << "\"\n";
    ofs << "[meta]\n";
    ofs << "M = " << cfg.M << "\n";
    ofs << "K = " << cfg.K << "\n";
    ofs << "N = " << cfg.N << "\n";
    ofs << "endian = \"" << cfg.endian << "\"\n";
    // model_cfg is stored in its own table so callers can reference the
    // platform config independently of meta fields.
    ofs << "[model_cfg]\n";
    if (!cfg.model_cfg_path.empty()) {
        ofs << "path = \"" << cfg.model_cfg_path << "\"\n";
    } else {
        ofs << "path = \"model_cfg.toml\"\n";
    }
    return ofs.good();
}

// read_case_toml uses the project's mini TOML parser (parse_toml_file)
// to extract keys; we do not need ad-hoc line-based parsers here.

bool read_case_toml(const std::string &path, CaseConfig &out) {
    // Use the project's mini TOML parser for robust key extraction.
    auto map = parse_toml_file(path);
    if (map.empty()) return false;
    out.case_path = path;
    auto get = [&](const std::string &k)->std::string{
        auto it = map.find(k);
        if (it == map.end()) return std::string();
        return it->second;
    };
    // keys are lowercased by parser
    out.a_path = get("input.a.path");
    out.b_path = get("input.b.path");
    out.c_golden_path = get("output.c.golden");
    out.c_out_path = get("output.c.out");
    // If TOML uses relative paths, resolve them against PROJECT_SRC_DIR so
    // tests and AIC can find files regardless of the current working dir.
    try {
        if (!out.a_path.empty()) {
            std::filesystem::path p(out.a_path);
            if (p.is_relative()) out.a_path = (std::filesystem::path(PROJECT_SRC_DIR) / p).string();
        }
        if (!out.b_path.empty()) {
            std::filesystem::path p(out.b_path);
            if (p.is_relative()) out.b_path = (std::filesystem::path(PROJECT_SRC_DIR) / p).string();
        }
        if (!out.c_golden_path.empty()) {
            std::filesystem::path p(out.c_golden_path);
            if (p.is_relative()) out.c_golden_path = (std::filesystem::path(PROJECT_SRC_DIR) / p).string();
        }
        if (!out.c_out_path.empty()) {
            std::filesystem::path p(out.c_out_path);
            if (p.is_relative()) out.c_out_path = (std::filesystem::path(PROJECT_SRC_DIR) / p).string();
        }
        // resolve model_cfg if present in its own table
        out.model_cfg_path = get("model_cfg.path");
        if (!out.model_cfg_path.empty()) {
            std::filesystem::path p(out.model_cfg_path);
            if (p.is_relative()) out.model_cfg_path = (std::filesystem::path(PROJECT_SRC_DIR) / p).string();
        }
        // make case_path absolute as well
        std::filesystem::path cp(path);
        if (cp.is_relative()) out.case_path = (std::filesystem::path(PROJECT_SRC_DIR) / cp).string();
        else out.case_path = path;
    } catch(...) {
        // ignore filesystem errors; keep original values if resolution fails
    }
    auto sa = get("input.a.addr"); if (!sa.empty()) out.a_addr = static_cast<uint32_t>(std::stoul(sa));
    auto sb = get("input.b.addr"); if (!sb.empty()) out.b_addr = static_cast<uint32_t>(std::stoul(sb));
    auto sc = get("output.c.addr"); if (!sc.empty()) out.c_addr = static_cast<uint32_t>(std::stoul(sc));
    auto ta = get("input.a.type"); if (!ta.empty()) out.a_type = ta;
    auto tb = get("input.b.type"); if (!tb.empty()) out.b_type = tb;
    auto tc = get("output.c.type"); if (!tc.empty()) out.c_type = tc;
    auto mm = get("meta.m"); if (!mm.empty()) out.M = std::stoi(mm);
    auto kk = get("meta.k"); if (!kk.empty()) out.K = std::stoi(kk);
    auto nn = get("meta.n"); if (!nn.empty()) out.N = std::stoi(nn);
    auto endian = get("meta.endian"); if (!endian.empty()) out.endian = endian;
    return true;
}

bool create_cube_case_config(const std::string &case_toml, CaseConfig &cfg,
                       const std::string &case_dir, const std::string &base_name,
                       const std::vector<int16_t> &A, const std::vector<int16_t> &B,
                       int M, int K, int N) {
    try {
        std::filesystem::create_directories(case_dir);
    } catch(...) {
        // ignore
    }
    cfg.case_path = case_toml;
    cfg.a_path = case_dir + std::string("/") + base_name + std::string("_A.bin");
    cfg.b_path = case_dir + std::string("/") + base_name + std::string("_B.bin");
    cfg.c_golden_path = case_dir + std::string("/") + base_name + std::string("_C_golden.bin");
    cfg.c_out_path = case_dir + std::string("/") + base_name + std::string("_C_out.bin");
    cfg.a_addr = 0;
    cfg.b_addr = static_cast<uint32_t>(A.size());
    cfg.c_addr = static_cast<uint32_t>(A.size() + B.size());
    cfg.M = M; cfg.K = K; cfg.N = N;
    // reference the shared model config by default (relative path)
    cfg.model_cfg_path = std::string("model_cfg.toml");

    if (!write_bin_int16(cfg.a_path, A)) return false;
    if (!write_bin_int16(cfg.b_path, B)) return false;

    std::vector<int32_t> Cgold(static_cast<size_t>(M) * static_cast<size_t>(N), 0);
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            int32_t acc = 0;
            for (int k = 0; k < K; ++k) {
                acc += static_cast<int32_t>(A[i*K + k]) * static_cast<int32_t>(B[k*N + j]);
            }
            Cgold[i* N + j] = acc;
        }
    }
    if (!write_bin_int32(cfg.c_golden_path, Cgold)) return false;
    return write_case_toml(cfg);
}

} // namespace util
