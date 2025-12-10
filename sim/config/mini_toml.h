#ifndef MINI_TOML_H
#define MINI_TOML_H

#include <string>
#include <unordered_map>

// Very small TOML-ish parser for the project's config needs.
// Supports:
// - [section] and nested [a.b] sections
// - dotted keys: key = value or a.b = value
// - quoted strings, integers, booleans (true/false)
// - comments with '#'
// This is NOT a full TOML implementation; it's a pragmatic parser for simple configs.

std::unordered_map<std::string, std::string> parse_toml_file(const std::string& path);

#endif // MINI_TOML_H
