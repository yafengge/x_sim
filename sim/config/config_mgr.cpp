#include "config/config_mgr.h"
#include "config/toml_adapter.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <mutex>

// 文件：config/config_mgr.cpp
// 说明：实现配置管理器的缓存与按键读取逻辑。主要职责：
// - 按文件路径缓存解析结果并根据文件 mtime 刷新。
// - 提供线程安全的按键读取接口（string/int/bool/dataflow）。

namespace config_mgr {

Manager::Manager() {}
Manager::~Manager() {}

// 将键归一化为小写形式以保证一致性
std::string Manager::normalize_key(const std::string &key) {
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c){ return std::tolower(c); });
    return k;
}

// 若缓存中不存在或文件已修改，则从磁盘解析并加载到缓存中
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
        e.map = toml_adapter::parse_toml_file(path);
        e.mtime = mtime;
        cache_[path] = std::move(e);
    }
}

// 从缓存中按键读取字符串值（成功返回 true）
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

// 读取并尝试转换为整数
bool Manager::get_int(const std::string &path, const std::string &key, int &out) const {
    std::string v;
    if (!get_string(path, key, v)) return false;
    try { out = std::stoi(v); return true; } catch(...) { return false; }
}

// 读取并尝试解析为布尔值（支持 true/false/1/0/yes/no/on/off）
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

// 便捷包装器函数，供其他模块直接调用
bool get_string_key(const std::string &path, const std::string &dotted_key, std::string &out) {
    return mgr().get_string(path, dotted_key, out);
}

bool get_int_key(const std::string &path, const std::string &dotted_key, int &out) {
    return mgr().get_int(path, dotted_key, out);
}

bool get_bool_key(const std::string &path, const std::string &dotted_key, bool &out) {
    return mgr().get_bool(path, dotted_key, out);
}

// 将字符串形式的 dataflow 映射到枚举值
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
