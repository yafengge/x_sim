#include "config/config.h"
#include <algorithm>
#include <cctype>

namespace {
static inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool lookup_int(const config::TomlParser::map_t &m, const std::string &k1, const std::string &k2, int &out) {
    auto it = m.find(k1);
    if (it == m.end()) it = m.find(k2);
    if (it == m.end()) return false;
    try { out = std::stoi(it->second); return true; } catch(...) { return false; }
}

static bool lookup_string(const config::TomlParser::map_t &m, const std::string &k1, const std::string &k2, std::string &out) {
    auto it = m.find(k1);
    if (it == m.end()) it = m.find(k2);
    if (it == m.end()) return false;
    out = it->second;
    return true;
}
}

namespace config {

std::optional<Config> Config::from_map(const TomlParser::map_t &m, std::string *err_msg) {
    Config c;
    // array rows/cols may be under `array_rows` or `cube.array_rows` (keys are lowercased)
    if (!lookup_int(m, "cube.array_rows", "array_rows", c.array_rows)) {
        if (err_msg) *err_msg = "missing array_rows";
        return std::nullopt;
    }
    if (!lookup_int(m, "cube.array_cols", "array_cols", c.array_cols)) {
        if (err_msg) *err_msg = "missing array_cols";
        return std::nullopt;
    }
    // optional tile sizes
    (void) lookup_int(m, "cube.tile_rows", "tile_rows", c.tile_rows);
    (void) lookup_int(m, "cube.tile_cols", "tile_cols", c.tile_cols);

    std::string df;
    if (lookup_string(m, "cube.dataflow", "dataflow", df)) {
        std::string up = df; std::transform(up.begin(), up.end(), up.begin(), [](unsigned char ch){ return static_cast<char>(std::toupper(ch)); });
        if (up == "WEIGHT_STATIONARY") c.dataflow = Dataflow::WEIGHT_STATIONARY;
        else if (up == "OUTPUT_STATIONARY") c.dataflow = Dataflow::OUTPUT_STATIONARY;
        else if (up == "INPUT_STATIONARY") c.dataflow = Dataflow::INPUT_STATIONARY;
        else {
            if (err_msg) *err_msg = "unknown dataflow: " + df;
            return std::nullopt;
        }
    }

    return c;
}

} // namespace config
