#include "config/mini_toml.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// 文件：config/mini_toml.cpp
// 说明：极简 TOML 解析器的实现。将配置文件解析为小型的 string->string map，
// 供 `config_mgr` 与其他模块按键读取配置值。

static inline std::string trim(const std::string &s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

// Parse a simple TOML-like file into a flat dotted-key map. Comments starting
// with '#' are ignored. Section headers ([a] or [a.b]) prefix subsequent keys.
std::unordered_map<std::string, std::string> parse_toml_file(const std::string& path) {
    std::unordered_map<std::string, std::string> out;
    std::ifstream fin(path);
    if (!fin) return out;

    std::string line;
    std::string cur_section;
    while (std::getline(fin, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        line = trim(line);
        if (line.empty()) continue;
        if (line.front() == '[' && line.back() == ']') {
            cur_section = trim(line.substr(1, line.size() - 2));
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        // remove surrounding quotes
        if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
            val = val.substr(1, val.size() - 2);
        }
        // Compose full dotted key using current section if present
        std::string full = key;
        if (!cur_section.empty()) {
            full = cur_section + "." + key;
        }
        out[to_lower(full)] = val;
    }
    return out;
}
