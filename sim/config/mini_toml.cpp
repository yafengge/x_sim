#include "config/mini_toml.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

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
        // if key contains dots like a.b, keep as-is; if section present, prefix
        std::string full = key;
        if (!cur_section.empty()) {
            // prefix to support nested semantics
            if (key.find('.') == std::string::npos) full = cur_section + "." + key;
            else full = cur_section + "." + key;
        }
        out[to_lower(full)] = val;
    }
    return out;
}
