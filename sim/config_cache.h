#ifndef CONFIG_CACHE_H
#define CONFIG_CACHE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <shared_mutex>
#include "mini_toml.h"


// Simple thread-safe config cache with automatic reload on file change.
class ConfigCache {
public:
    static ConfigCache& instance();

    // Getters return true if value found and parsed, false otherwise.
    bool get_int(const std::string& path, const std::string& key, int &out) const;
    bool get_bool(const std::string& path, const std::string& key, bool &out) const;
    bool get_string(const std::string& path, const std::string& key, std::string &out) const;
    // dataflow is a small symbolic string mapping; caller handles mapping if needed
    bool get_dataflow(const std::string& path, const std::string& key, std::string &out) const;

    // Force reload (useful for tests)
    void reload_if_needed(const std::string& path) const;

private:
    ConfigCache();
    ~ConfigCache();

    struct CacheEntry {
        std::unordered_map<std::string, std::string> map;
        std::filesystem::file_time_type mtime;
    };

    // mutable to allow reload in const getters
    mutable std::unordered_map<std::string, CacheEntry> cache_;
    mutable std::shared_mutex lock_;

    void ensure_loaded(const std::string& path) const;
    static std::string normalize_key(const std::string& key);
};

#endif // CONFIG_CACHE_H
