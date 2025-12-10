#include "config_cache.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <shared_mutex>

using namespace std::literals;

ConfigCache& ConfigCache::instance() {
    static ConfigCache inst;
    return inst;
}

ConfigCache::ConfigCache() {}
ConfigCache::~ConfigCache() {}

void ConfigCache::ensure_loaded(const std::string& path) const {
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

void ConfigCache::reload_if_needed(const std::string& path) const {
    ensure_loaded(path);
}

// parse_file removed: using mini_toml::parse_toml_file instead

std::string ConfigCache::normalize_key(const std::string& key) {
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c){ return std::tolower(c); });
    return k;
}

bool ConfigCache::get_string(const std::string& path, const std::string& key, std::string &out) const {
    ensure_loaded(path);
    std::shared_lock<std::shared_mutex> r(lock_);
    auto it = cache_.find(path);
    if (it == cache_.end()) return false;
    auto k = normalize_key(key);
    auto fit = it->second.map.find(k);
    if (fit == it->second.map.end()) return false;
    out = fit->second;
    return true;
}

bool ConfigCache::get_int(const std::string& path, const std::string& key, int &out) const {
    std::string v;
    if (!get_string(path, key, v)) return false;
    try {
        out = std::stoi(v);
        return true;
    } catch (...) { return false; }
}

bool ConfigCache::get_bool(const std::string& path, const std::string& key, bool &out) const {
    std::string v;
    if (!get_string(path, key, v)) return false;
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return std::tolower(c); });
    if (v == "true" || v == "1" || v == "yes" || v == "on") { out = true; return true; }
    if (v == "false" || v == "0" || v == "no" || v == "off") { out = false; return true; }
    return false;
}

bool ConfigCache::get_dataflow(const std::string& path, const std::string& key, std::string &out) const {
    return get_string(path, key, out);
}
