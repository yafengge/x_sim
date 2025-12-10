#include "config_loader.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {

enum class Section { None, Cube, Memory };

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

Section parse_section(const std::string& raw) {
    const auto name = to_lower(trim(raw));
    if (name == "cube") return Section::Cube;
    if (name == "memory") return Section::Memory;
    return Section::None;
}

bool parse_bool(const std::string& v, bool& out) {
    if (v == "true") { out = true; return true; }
    if (v == "false") { out = false; return true; }
    return false;
}

bool parse_int(const std::string& v, int& out) {
    try {
        size_t idx = 0;
        int val = std::stoi(v, &idx, 0);
        if (idx == v.size()) { out = val; return true; }
    } catch (...) {}
    return false;
}

bool parse_dataflow(const std::string& v, Dataflow& out) {
    if (v == "WEIGHT_STATIONARY") { out = Dataflow::WEIGHT_STATIONARY; return true; }
    if (v == "OUTPUT_STATIONARY") { out = Dataflow::OUTPUT_STATIONARY; return true; }
    if (v == "INPUT_STATIONARY") { out = Dataflow::INPUT_STATIONARY; return true; }
    return false;
}

bool load_config_sections(const std::string& path, CubeConfig& cube_cfg, MemConfig& mem_cfg, bool want_cube, bool want_memory, std::string* err) {
    std::ifstream fin(path);
    if (!fin) {
        if (err) *err = "Failed to open config file: " + path;
        return false;
    }

    std::string line;
    int line_no = 0;
    Section section = Section::None;

    while (std::getline(fin, line)) {
        ++line_no;
        auto hash_pos = line.find('#');
        if (hash_pos != std::string::npos) line = line.substr(0, hash_pos);
        line = trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            section = parse_section(line.substr(1, line.size() - 2));
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) {
            if (err) *err = "Line " + std::to_string(line_no) + ": missing '='";
            return false;
        }

        std::string key = trim(line.substr(0, eq));
        std::string val_raw = trim(line.substr(eq + 1));

        // support dotted keys like "cube.array_rows" or "memory.memory_latency"
        bool has_dot = false;
        std::string top_key;
        std::string sub_key;
        auto dotpos = key.find('.');
        if (dotpos != std::string::npos) {
            has_dot = true;
            top_key = to_lower(trim(key.substr(0, dotpos)));
            sub_key = trim(key.substr(dotpos + 1));
        }

        const bool in_cube = ((section == Section::Cube) && want_cube) || (has_dot && top_key == "cube" && want_cube);
        const bool in_mem  = ((section == Section::Memory) && want_memory) || (has_dot && top_key == "memory" && want_memory);
        if (!in_cube && !in_mem) continue; // skip keys outside requested sections or unknown prefixes

        // if dotted, prefer the sub_key as the actual key name
        if (has_dot) key = sub_key;
        key = to_lower(key);
        // strip quotes from value if present
        if (val_raw.size() >= 2 && ((val_raw.front() == '"' && val_raw.back() == '"') || (val_raw.front() == '\'' && val_raw.back() == '\''))) {
            val_raw = val_raw.substr(1, val_raw.size() - 2);
        }
        if (val_raw.size() >= 2 && ((val_raw.front() == '"' && val_raw.back() == '"') || (val_raw.front() == '\'' && val_raw.back() == '\''))) {
            val_raw = val_raw.substr(1, val_raw.size() - 2);
        }

        int iv = 0; bool bv = false; Dataflow df;

        if (in_cube) {
            if (key == "array_rows") {
                if (!parse_int(val_raw, iv)) goto parse_error;
                cube_cfg.array_rows = iv;
            } else if (key == "array_cols") {
                if (!parse_int(val_raw, iv)) goto parse_error;
                cube_cfg.array_cols = iv;
            } else if (key == "pe_latency") {
                if (!parse_int(val_raw, iv)) goto parse_error;
                cube_cfg.pe_latency = iv;
            } else if (key == "progress_interval") {
                if (!parse_int(val_raw, iv)) goto parse_error;
                cube_cfg.progress_interval = iv;
            } else if (key == "verbose") {
                if (!parse_bool(val_raw, bv)) goto parse_error;
                cube_cfg.verbose = bv;
            } else if (key == "trace_cycles") {
                if (!parse_int(val_raw, iv)) goto parse_error;
                cube_cfg.trace_cycles = iv;
            } else if (key == "dataflow") {
                if (!parse_dataflow(val_raw, df)) goto parse_error;
                cube_cfg.dataflow = df;
            } else {
                // Unknown cube keys ignored
            }
        } else if (in_mem) {
            if (key == "memory_latency") {
                if (!parse_int(val_raw, iv)) goto parse_error;
                mem_cfg.memory_latency = iv;
            } else if (key == "bandwidth") {
                if (!parse_int(val_raw, iv)) goto parse_error;
                mem_cfg.bandwidth = iv;
            } else if (key == "max_outstanding") {
                if (!parse_int(val_raw, iv)) goto parse_error;
                mem_cfg.max_outstanding = iv;
            } else {
                // Unknown memory keys ignored
            }
        }
        continue;

    parse_error:
        if (err) *err = "Line " + std::to_string(line_no) + ": invalid value for '" + key + "'";
        return false;
    }

    return true;
}

} // namespace

bool load_config(const std::string& path, CubeConfig& cube_cfg, MemConfig& mem_cfg, std::string* err) {
    return load_config_sections(path, cube_cfg, mem_cfg, /*want_cube=*/true, /*want_memory=*/true, err);
}

bool load_cube_config(const std::string& path, CubeConfig& cfg, std::string* err) {
    MemConfig dummy;
    return load_config_sections(path, cfg, dummy, /*want_cube=*/true, /*want_memory=*/false, err);
}

bool load_memory_config(const std::string& path, MemConfig& cfg, std::string* err) {
    CubeConfig dummy;
    return load_config_sections(path, dummy, cfg, /*want_cube=*/false, /*want_memory=*/true, err);
}
