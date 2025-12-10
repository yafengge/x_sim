#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include "systolic_common.h"

// Load SystolicConfig from a TOML-like file. Returns true on success.
// Full load parses all sections; helpers load only the requested section.
bool load_config(const std::string& path, SystolicConfig& cfg, std::string* err = nullptr);
bool load_cube_config(const std::string& path, SystolicConfig& cfg, std::string* err = nullptr);
bool load_memory_config(const std::string& path, SystolicConfig& cfg, std::string* err = nullptr);

#endif // CONFIG_LOADER_H
