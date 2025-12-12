#ifndef CONFIG_CONFIG_MGR_H
#define CONFIG_CONFIG_MGR_H

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <filesystem>
#include "types.h"

// 文件：config/config_mgr.h
// 说明：配置管理器（缓存层）。
// - 提供按文件路径读取配置键的线程安全缓存（按文件 mtime 刷新）。
// - 将解析后的键值对以小写 dotted-key 存储，供上层按需读取。
// - 提供便捷的 get_string/get_int/get_bool/get_dataflow 接口。

namespace config_mgr {

struct CacheEntry {
    std::unordered_map<std::string, std::string> map;
    std::filesystem::file_time_type mtime;
};

class Manager {
public:
    Manager();
    ~Manager();

    // Ensure the file at `path` is loaded into the cache (or refreshed if modified)
    void ensure_loaded(const std::string &path) const;

    // Typed getters return true on success and write the parsed/converted value to `out`.
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

// Convenience wrappers used by other modules (lower-level API)
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
