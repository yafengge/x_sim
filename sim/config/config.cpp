// Combined implementation of toml parser, Config mapping, and provider
#include "config/config.h"
#include "util/utils.h"
#include <toml++/toml.h>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
#include <shared_mutex>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>

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
        out[util::to_lower(prefix)] = s;
        return;
    }

    if (auto sv = node.as_string()) { out[util::to_lower(prefix)] = sv->get(); return; }
    if (auto iv = node.as_integer()) { out[util::to_lower(prefix)] = std::to_string(iv->get()); return; }
    if (auto fv = node.as_floating_point()) { out[util::to_lower(prefix)] = std::to_string(fv->get()); return; }
    if (auto bv = node.as_boolean()) { out[util::to_lower(prefix)] = bv->get() ? "true" : "false"; return; }

    std::ostringstream oss;
    oss << toml::node_view{node};
    out[util::to_lower(prefix)] = oss.str();
}

namespace config {

// TomlParser implementation
config::TomlParser::map_t TomlParser::parse_file(const std::string &path) noexcept {
    map_t out;
    try {
        auto doc = toml::parse_file(path);
        flatten_toml(doc, "", out);
    } catch(...) {}
    return out;
}

// Config mapping helpers
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

std::optional<Config> Config::from_map(const TomlParser::map_t &m, std::string *err_msg) {
    Config c;
    if (!lookup_int(m, "cube.array_rows", "array_rows", c.array_rows)) {
        if (err_msg) *err_msg = "missing array_rows";
        return std::nullopt;
    }
    if (!lookup_int(m, "cube.array_cols", "array_cols", c.array_cols)) {
        if (err_msg) *err_msg = "missing array_cols";
        return std::nullopt;
    }
    (void) lookup_int(m, "cube.tile_rows", "tile_rows", c.tile_rows);
    (void) lookup_int(m, "cube.tile_cols", "tile_cols", c.tile_cols);

    std::string df;
    if (lookup_string(m, "cube.dataflow", "dataflow", df)) {
        std::string up = util::to_upper(df);
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

// Provider implementation (DefaultProvider + helpers)
class DefaultProvider : public IConfigProvider {
public:
    DefaultProvider() = default;
    std::optional<std::string> get_raw(const std::string &path, const std::string &dotted_key) override {
        ensure_loaded(path);
        std::shared_lock<std::shared_mutex> r(lock_);
        auto it = cache_.find(path);
        if (it == cache_.end()) return std::nullopt;
        std::string k = util::to_lower(dotted_key);
        auto fit = it->second.map.find(k);
        if (fit == it->second.map.end()) return std::nullopt;
        return fit->second;
    }

    std::optional<config::Config> load_config(const std::string &path, std::string *err = nullptr) override {
        ensure_loaded(path);
        std::shared_lock<std::shared_mutex> r(lock_);
        auto it = cache_.find(path);
        if (it == cache_.end()) {
            if (err) *err = "no such config";
            return std::nullopt;
        }
        std::string e;
        auto cfg = config::Config::from_map(it->second.map, &e);
        if (!cfg.has_value()) {
            if (err) *err = e;
            return std::nullopt;
        }
        return cfg;
    }

private:
    struct Entry { std::unordered_map<std::string, std::string> map; std::filesystem::file_time_type mtime{}; };

    void ensure_loaded(const std::string &path) {
        namespace fs = std::filesystem;
        std::unique_lock<std::shared_mutex> w(lock_);
        auto it = cache_.find(path);
        fs::file_time_type mtime{};
        try { if (fs::exists(path)) mtime = fs::last_write_time(path); } catch(...) {}
        if (it == cache_.end() || it->second.mtime != mtime) {
            Entry e;
            e.map = config::TomlParser::parse_file(path);
            e.mtime = mtime;
            cache_[path] = std::move(e);
        }
    }

    mutable std::unordered_map<std::string, Entry> cache_;
    mutable std::shared_mutex lock_;
};

static std::shared_ptr<IConfigProvider> global_provider = nullptr;
static std::mutex provider_mutex;

// default path storage
static std::string global_default_path = "model_cfg.toml";
static std::mutex global_default_path_mutex;

void set_default_path(const std::string &p) {
    std::lock_guard<std::mutex> lk(global_default_path_mutex);
    global_default_path = p;
}

std::string get_default_path() {
    std::lock_guard<std::mutex> lk(global_default_path_mutex);
    return global_default_path;
}

void set_provider(std::shared_ptr<IConfigProvider> p) {
    std::lock_guard<std::mutex> lk(provider_mutex);
    global_provider = p;
}

std::shared_ptr<IConfigProvider> get_provider() {
    std::lock_guard<std::mutex> lk(provider_mutex);
    if (!global_provider) global_provider = std::make_shared<DefaultProvider>();
    return global_provider;
}

// conversion helpers
template<typename T>
static std::optional<T> convert_from_string(const std::string &s) { return std::nullopt; }

template<>
std::optional<std::string> convert_from_string<std::string>(const std::string &s) { return s; }

template<>
std::optional<int> convert_from_string<int>(const std::string &s) { try { return std::stoi(s); } catch(...) { return std::nullopt; } }

template<>
std::optional<bool> convert_from_string<bool>(const std::string &s) {
    std::string v = util::to_lower(s);
    if (v == "true" || v == "1" || v == "yes" || v == "on") return true;
    if (v == "false" || v == "0" || v == "no" || v == "off") return false;
    return std::nullopt;
}

template<>
std::optional<double> convert_from_string<double>(const std::string &s) { try { return std::stod(s); } catch(...) { return std::nullopt; } }

template<>
std::optional<config::Config> convert_from_string<config::Config>(const std::string &s) { (void)s; return std::nullopt; }

template<>
std::optional<Dataflow> convert_from_string<Dataflow>(const std::string &s) {
    std::string up = util::to_upper(s);
    if (up == "WEIGHT_STATIONARY") return Dataflow::WEIGHT_STATIONARY;
    if (up == "OUTPUT_STATIONARY") return Dataflow::OUTPUT_STATIONARY;
    if (up == "INPUT_STATIONARY") return Dataflow::INPUT_STATIONARY;
    return std::nullopt;
}

template<typename T>
std::optional<T> get_impl(const std::string &dotted_key, const std::string &path) {
    auto prov = get_provider();
    if constexpr (std::is_same_v<T, config::Config>) {
        std::string err; return prov->load_config(path, &err);
    }
    if (auto raw = prov->get_raw(path, dotted_key)) {
        if (auto conv = convert_from_string<T>(*raw)) return conv;
    }
    auto pos = dotted_key.find_last_of('.');
    if (pos != std::string::npos) {
        std::string short_key = dotted_key.substr(pos+1);
        if (auto raw2 = prov->get_raw(path, short_key)) {
            if (auto conv2 = convert_from_string<T>(*raw2)) return conv2;
        }
    }
    return std::nullopt;
}

// explicit instantiations
template std::optional<int> get<int>(const std::string&, const std::string&);
template std::optional<bool> get<bool>(const std::string&, const std::string&);
template std::optional<double> get<double>(const std::string&, const std::string&);
template std::optional<std::string> get<std::string>(const std::string&, const std::string&);
template std::optional<Dataflow> get<Dataflow>(const std::string&, const std::string&);
template std::optional<config::Config> get<config::Config>(const std::string&, const std::string&);

template<typename T>
std::optional<T> get(const std::string &dotted_key, const std::string &path) { return get_impl<T>(dotted_key, path); }

} // namespace config
