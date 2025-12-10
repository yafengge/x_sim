#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include "systolic_common.h"

// Load configuration from a TOML-like file. Returns true on success.
// Full load parses both cube and memory sections; helpers load only the requested section.
bool load_config(const std::string& path, CubeConfig& cube_cfg, MemConfig& mem_cfg, std::string* err = nullptr);
bool load_cube_config(const std::string& path, CubeConfig& cfg, std::string* err = nullptr);
bool load_memory_config(const std::string& path, MemConfig& cfg, std::string* err = nullptr);

// Per-key getters (no caching): each call reads and parses the file.
// Returns true if the key was found and parsed into 'out'.
bool get_int_key(const std::string& path, const std::string& dotted_key, int& out);
bool get_bool_key(const std::string& path, const std::string& dotted_key, bool& out);
bool get_string_key(const std::string& path, const std::string& dotted_key, std::string& out);
bool get_dataflow_key(const std::string& path, const std::string& dotted_key, Dataflow& out);

#endif // CONFIG_LOADER_H
