#ifndef CONFIG_CONFIG_MGR_H
#define CONFIG_CONFIG_MGR_H

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <filesystem>
#include "types.h"

namespace config_mgr {

struct CacheEntry {
    std::unordered_map<std::string, std::string> map;
    std::filesystem::file_time_type mtime;
};

class Manager {
public:
    Manager();
    ~Manager();

    void ensure_loaded(const std::string &path) const;

    bool get_string(const std::string &path, const std::string &key, std::string &out) const;
    bool get_int(const std::string &path, const std::string &key, int &out) const;
    bool get_bool(const std::string &path, const std::string &key, bool &out) const;
    bool get_dataflow(const std::string &path, const std::string &key, std::string &out) const;

private:
    static std::string normalize_key(const std::string &key);
    mutable std::unordered_map<std::string, CacheEntry> cache_;
    mutable std::shared_mutex lock_;
};

Manager& mgr();

bool get_string_key(const std::string &path, const std::string &dotted_key, std::string &out);
bool get_int_key(const std::string &path, const std::string &dotted_key, int &out);
bool get_bool_key(const std::string &path, const std::string &dotted_key, bool &out);
bool get_dataflow_key(const std::string &path, const std::string &dotted_key, Dataflow &out);

} // namespace config_mgr

using config_mgr::get_int_key;
using config_mgr::get_bool_key;
using config_mgr::get_string_key;
using config_mgr::get_dataflow_key;

#endif // CONFIG_CONFIG_MGR_H
