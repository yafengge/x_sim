#include "config/toml_parser.h"
#include <toml++/toml.h>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
static inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

using map_t = std::unordered_map<std::string, std::string>;

static void flatten_toml(const toml::node& node, const std::string& prefix, map_t &out) {
    if (auto tbl = node.as_table()) {
        for (const auto &kv : *tbl) {
            std::string key = prefix.empty() ? std::string(kv.first) : prefix + "." + std::string(kv.first);
            flatten_toml(kv.second, key, out);
        }
        return;
    }

    if (auto arr = node.as_array()) {
        std::string s;
        for (size_t i = 0; i < arr->size(); ++i) {
            if (i) s += ",";
            const toml::node* elem = arr->get(i);
            if (!elem) continue;
            if (auto sv = elem->as_string()) s += sv->get();
            else if (auto iv = elem->as_integer()) s += std::to_string(iv->get());
            else if (auto fv = elem->as_floating_point()) s += std::to_string(fv->get());
            else if (auto bv = elem->as_boolean()) s += (bv->get() ? "true" : "false");
            else {
                std::ostringstream oss;
                oss << toml::node_view{*elem};
                s += oss.str();
            }
        }
        out[to_lower(prefix)] = s;
        return;
    }

    if (auto sv = node.as_string()) { out[to_lower(prefix)] = sv->get(); return; }
    if (auto iv = node.as_integer()) { out[to_lower(prefix)] = std::to_string(iv->get()); return; }
    if (auto fv = node.as_floating_point()) { out[to_lower(prefix)] = std::to_string(fv->get()); return; }
    if (auto bv = node.as_boolean()) { out[to_lower(prefix)] = bv->get() ? "true" : "false"; return; }

    std::ostringstream oss;
    oss << toml::node_view{node};
    out[to_lower(prefix)] = oss.str();
}

} // anonymous

namespace config {

config::TomlParser::map_t TomlParser::parse_file(const std::string &path) noexcept {
    map_t out;
    try {
        auto doc = toml::parse_file(path);
        flatten_toml(doc, "", out);
    } catch(...) {}
    return out;
}

} // namespace config
