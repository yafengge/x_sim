#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include "types.h"


namespace config {

class TomlParser {
public:
    using map_t = std::unordered_map<std::string, std::string>;
    // Parse a TOML file and return a flat dotted-key -> string map
    // (keys normalized to lower-case). Implementation is in `config/config.cpp`.
    static map_t parse_file(const std::string &path) noexcept;
};

struct Config {
    int array_rows = 0;
    int array_cols = 0;
    int tile_rows = 0;
    int tile_cols = 0;
    Dataflow dataflow = Dataflow::WEIGHT_STATIONARY;

    // Parse from a flat map produced by TomlParser. Returns std::nullopt on error.
    // Implementation lives in `config/config.cpp`.
    static std::optional<Config> from_map(const TomlParser::map_t &m, std::string *err_msg = nullptr);
};

class IConfigProvider {
public:
    virtual ~IConfigProvider() = default;
    virtual std::optional<std::string> get_raw(const std::string &path, const std::string &dotted_key) = 0;
    virtual std::optional<Config> load_config(const std::string &path, std::string *err = nullptr) { (void)path; if (err) *err = "not-implemented"; return std::nullopt; }
};

// Provider management
void set_provider(std::shared_ptr<IConfigProvider> p);
std::shared_ptr<IConfigProvider> get_provider();

// Runtime default path handling. Call `set_default_path` at program
// initialization (e.g. in tests) so callers may rely on a shared default.
void set_default_path(const std::string &p);
std::string get_default_path();

// Free-function API: get<T>(dotted_key, path)
template<typename T>
std::optional<T> get(const std::string &dotted_key, const std::string &path = "model_cfg.toml");

} // namespace config

// Lightweight global wrapper for callers that prefer a short call-site API.
// Usage: `auto v = get<int>("cube.array_rows");`
template<typename T>
inline std::optional<T> get(const std::string &key) {
    return config::get<T>(key, config::get_default_path());
}

template<typename T>
inline std::optional<T> get(const std::string &key, const std::string &path) {
    return config::get<T>(key, path);
}

#endif // CONFIG_CONFIG_H
