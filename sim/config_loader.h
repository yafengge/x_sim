#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include "types.h"
#include "config_cache.h"

// Per-key getters backed by a global ConfigCache (thread-safe, auto-reload).
// Returns true if the key was found and parsed into 'out'.
bool get_int_key(const std::string& path, const std::string& dotted_key, int& out);
bool get_bool_key(const std::string& path, const std::string& dotted_key, bool& out);
bool get_string_key(const std::string& path, const std::string& dotted_key, std::string& out);
bool get_dataflow_key(const std::string& path, const std::string& dotted_key, Dataflow& out);

#endif // CONFIG_LOADER_H
