#include "config/toml_adapter.h"

#include <algorithm>
#include <cctype>

#ifndef HAVE_TOMLPP
#include "config/mini_toml.h"

namespace toml_adapter {

std::unordered_map<std::string, std::string> parse_toml_file(const std::string& path) {
    // 回退到项目内置的极简解析器
    return ::parse_toml_file(path);
}

} // namespace toml_adapter

#else // HAVE_TOMLPP

#include <toml++/toml.h>
#include <sstream>

namespace {
static inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}
}

namespace toml_adapter {

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

    // Value or other node: extract native value where possible
    if (auto sv = node.as_string()) {
        out[to_lower(prefix)] = sv->get();
        return;
    }
    if (auto iv = node.as_integer()) {
        out[to_lower(prefix)] = std::to_string(iv->get());
        return;
    }
    if (auto fv = node.as_floating_point()) {
        out[to_lower(prefix)] = std::to_string(fv->get());
        return;
    }
    if (auto bv = node.as_boolean()) {
        out[to_lower(prefix)] = bv->get() ? "true" : "false";
        return;
    }
    // Fallback: stringify via node_view
    {
        std::ostringstream oss;
        oss << toml::node_view{node};
        out[to_lower(prefix)] = oss.str();
    }
}

std::unordered_map<std::string, std::string> parse_toml_file(const std::string& path) {
    map_t out;
    try {
        auto doc = toml::parse_file(path);
        flatten_toml(doc, "", out);
    } catch(...) {
        // swallow parse errors and return empty map
    }
    return out;
}

} // namespace toml_adapter

#endif // HAVE_TOMLPP
