#include "util/case_io.h"
#include "config/mini_toml.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <unordered_map>

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
bool write_case_toml(const CaseConfig &cfg) {
    std::ofstream ofs(cfg.case_path);
    if (!ofs) return false;
    ofs << "# case toml generated\n";
    ofs << "[input.A]\n";
    ofs << "path = \"" << cfg.a_path << "\"\n";
    ofs << "addr = " << cfg.a_addr << "\n";
    ofs << "type = \"" << cfg.a_type << "\"\n";
    ofs << "[input.B]\n";
    ofs << "path = \"" << cfg.b_path << "\"\n";
    ofs << "addr = " << cfg.b_addr << "\n";
    ofs << "type = \"" << cfg.b_type << "\"\n";
    ofs << "[output.C]\n";
    ofs << "golden = \"" << cfg.c_golden_path << "\"\n";
    ofs << "out = \"" << cfg.c_out_path << "\"\n";
    ofs << "addr = " << cfg.c_addr << "\n";
    ofs << "type = \"" << cfg.c_type << "\"\n";
    ofs << "[meta]\n";
    ofs << "M = " << cfg.M << "\n";
    ofs << "K = " << cfg.K << "\n";
    ofs << "N = " << cfg.N << "\n";
    ofs << "endian = \"" << cfg.endian << "\"\n";
    return ofs.good();
}

// Very small TOML reader that looks for the keys we wrote above. Not a full TOML parser.
static bool extract_string(const std::string &line, const std::string &key, std::string &out) {
    auto pos = line.find(key);
    if (pos == std::string::npos) return false;
    auto eq = line.find('=', pos);
    if (eq == std::string::npos) return false;
    auto first = line.find('"', eq);
    if (first == std::string::npos) return false;
    auto second = line.find('"', first + 1);
    if (second == std::string::npos) return false;
    out = line.substr(first + 1, second - first - 1);
    return true;
}

static bool extract_uint(const std::string &line, const std::string &key, uint32_t &out) {
    auto pos = line.find(key);
    if (pos == std::string::npos) return false;
    auto eq = line.find('=', pos);
    if (eq == std::string::npos) return false;
    std::string val = line.substr(eq + 1);
    std::istringstream iss(val);
    uint32_t v; iss >> v;
    if (iss.fail()) return false;
    out = v;
    return true;
}

static bool extract_int(const std::string &line, const std::string &key, int &out) {
    auto pos = line.find(key);
    if (pos == std::string::npos) return false;
    auto eq = line.find('=', pos);
    if (eq == std::string::npos) return false;
    std::string val = line.substr(eq + 1);
    std::istringstream iss(val);
    int v; iss >> v;
    if (iss.fail()) return false;
    out = v;
    return true;
}

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

} // namespace util
