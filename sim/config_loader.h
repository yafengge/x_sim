#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include "systolic_common.h"

// Load configuration from a TOML-like file. Returns true on success.
// Full load parses both cube and memory sections; helpers load only the requested section.
bool load_config(const std::string& path, CubeConfig& cube_cfg, MemConfig& mem_cfg, std::string* err = nullptr);
bool load_cube_config(const std::string& path, CubeConfig& cfg, std::string* err = nullptr);
bool load_memory_config(const std::string& path, MemConfig& cfg, std::string* err = nullptr);

#endif // CONFIG_LOADER_H
