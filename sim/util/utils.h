#pragma once
#include <vector>
#include <string>
#include "util/case_io.h"
#include <optional>
#include <filesystem>
#include <fstream>

// Helpers moved from tests/utils.* to util/ to be shared by tests and tools.
// Verification utilities moved to util/verify.h/.cpp
#include "util/verify.h"

namespace util {

// use `util::write_case_toml` / `util::create_cube_case_config` from util/case_io.h

// Use template helpers `util::write_bin<int16_t>` / `util::read_bin<int16_t>`

// Header-only binary IO templates (moved from util/bin_io.h)
template<typename T>
inline bool write_bin(const std::string &path, const std::vector<T> &v) {
	std::ofstream ofs(path, std::ios::binary);
	if (!ofs) return false;
	ofs.write(reinterpret_cast<const char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(T)));
	return ofs.good();
}

template<typename T>
inline bool read_bin(const std::string &path, std::vector<T> &v) {
	std::ifstream ifs(path, std::ios::binary | std::ios::ate);
	if (!ifs) return false;
	std::streamsize sz = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	if (sz % sizeof(T) != 0) return false;
	size_t count = static_cast<size_t>(sz / sizeof(T));
	v.resize(count);
	ifs.read(reinterpret_cast<char*>(v.data()), sz);
	return ifs.good();
}

// Path helpers (moved from util/path.h/.cpp)
std::optional<std::filesystem::path> find_existing_path(const std::string &path, int max_up = 5);
std::string resolve_path(const std::string &path);

// Per-case TOML handling is provided by `util::CaseConfig` and the
// functions in util/case_io.h. Include that header so callers of
// util/utils.h can refer to `util::CaseConfig` without a separate include.

} // namespace util
