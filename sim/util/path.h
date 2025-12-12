#pragma once
#include <string>
#include <optional>
#include <filesystem>

namespace util {

// Try to locate an existing file given `path`.
// Search order:
// 1) If `path` exists as given, return it.
// 2) If `path` is relative, search upward from current working directory
//    up to `max_up` parent levels for a candidate `cwd / path`.
// 3) Check PROJECT_SRC_DIR / path.
// Returns the resolved existing path or nullopt.
std::optional<std::filesystem::path> find_existing_path(const std::string &path, int max_up = 5);

// Resolve a possibly-relative path to a canonical form used for writing outputs.
// If `path` is absolute, returns it; otherwise returns PROJECT_SRC_DIR/path.
std::string resolve_path(const std::string &path);

} // namespace util
