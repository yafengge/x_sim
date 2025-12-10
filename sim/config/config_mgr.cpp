#include "config/config_mgr.h"
#include "config/mini_toml.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <mutex>

namespace config_mgr {

Manager::Manager() {}
Manager::~Manager() {}

std::string Manager::normalize_key(const std::string &key) {
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c){ return std::tolower(c); });
    return k;
}

void Manager::ensure_loaded(const std::string &path) const {
    namespace fs = std::filesystem;
    std::unique_lock<std::shared_mutex> w(lock_);
    auto it = cache_.find(path);
    fs::file_time_type mtime{};
    try {
        if (fs::exists(path)) mtime = fs::last_write_time(path);
    } catch (...) {}
    if (it == cache_.end() || it->second.mtime != mtime) {
        CacheEntry e;
        e.map = parse_toml_file(path);
        e.mtime = mtime;
        cache_[path] = std::move(e);
    }
}

bool Manager::get_string(const std::string &path, const std::string &key, std::string &out) const {
    ensure_loaded(path);
    std::shared_lock<std::shared_mutex> r(lock_);
    auto it = cache_.find(path);
    if (it == cache_.end()) return false;
    std::string k = normalize_key(key);
    auto fit = it->second.map.find(k);
    if (fit == it->second.map.end()) return false;
    out = fit->second;
    return true;
}

bool Manager::get_int(const std::string &path, const std::string &key, int &out) const {
    std::string v;
    if (!get_string(path, key, v)) return false;
    try { out = std::stoi(v); return true; } catch(...) { return false; }
}

bool Manager::get_bool(const std::string &path, const std::string &key, bool &out) const {
    std::string v;
    if (!get_string(path, key, v)) return false;
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return std::tolower(c); });
    if (v == "true" || v == "1" || v == "yes" || v == "on") { out = true; return true; }
    if (v == "false" || v == "0" || v == "no" || v == "off") { out = false; return true; }
    return false;
}

bool Manager::get_dataflow(const std::string &path, const std::string &key, std::string &out) const {
    return get_string(path, key, out);
}

Manager& mgr() {
    static Manager instance;
    return instance;
}

bool get_string_key(const std::string &path, const std::string &dotted_key, std::string &out) {
    return mgr().get_string(path, dotted_key, out);
}

bool get_int_key(const std::string &path, const std::string &dotted_key, int &out) {
    return mgr().get_int(path, dotted_key, out);
}

bool get_bool_key(const std::string &path, const std::string &dotted_key, bool &out) {
    return mgr().get_bool(path, dotted_key, out);
}

bool get_dataflow_key(const std::string &path, const std::string &dotted_key, Dataflow &out) {
    std::string v;
    if (!mgr().get_dataflow(path, dotted_key, v)) return false;
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    if (v == "WEIGHT_STATIONARY") { out = Dataflow::WEIGHT_STATIONARY; return true; }
    if (v == "OUTPUT_STATIONARY") { out = Dataflow::OUTPUT_STATIONARY; return true; }
    if (v == "INPUT_STATIONARY") { out = Dataflow::INPUT_STATIONARY; return true; }
    return false;
}

} // namespace config_mgr
