// Utilities implementation
#include "utils.h"
#include <random>
#include <sstream>
#include <iostream>
#include <cmath>

namespace util {

// generate_random_matrix was moved to util/verify (kept as test helper there).

std::optional<std::filesystem::path> find_existing_path(const std::string &path, int max_up) {
    std::filesystem::path p(path);
    if (std::filesystem::exists(p)) return std::filesystem::absolute(p);

    if (p.is_relative()) {
        std::filesystem::path cwd = std::filesystem::current_path();
        for (int depth = 0; depth < max_up; ++depth) {
            std::filesystem::path candidate = cwd / path;
            if (std::filesystem::exists(candidate)) return std::filesystem::absolute(candidate);
            if (cwd.has_parent_path()) cwd = cwd.parent_path(); else break;
        }
    }
    return std::nullopt;
}

std::string resolve_path(const std::string &path) {
    std::filesystem::path p(path);
    if (p.is_absolute()) return p.string();
    return std::filesystem::absolute(p).string();
}

} // namespace util

