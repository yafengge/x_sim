#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include "systolic_common.h"

// Load SystolicConfig from a TOML-like file. Returns true on success.
bool load_config(const std::string& path, SystolicConfig& cfg, std::string* err = nullptr);

#endif // CONFIG_LOADER_H
