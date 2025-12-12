#pragma once
#include <vector>
#include <string>
#include "util/case_io.h"

// Helpers moved from tests/utils.* to util/ to be shared by tests and tools.
namespace util {

std::vector<int16_t> generate_random_matrix(int rows, int cols, int min_val = -128, int max_val = 127);

void write_config_file(const std::string& path, int array_rows, int array_cols);

// Save/load matrix in raw binary form (row-major int16_t). Returns true on success.
bool write_matrix_bin(const std::string& path, const std::vector<int16_t>& data);
bool read_matrix_bin(const std::string& path, std::vector<int16_t>& out);

// Per-case TOML handling is provided by `util::CaseConfig` and the
// functions in util/case_io.h. Include that header so callers of
// util/utils.h can refer to `util::CaseConfig` without a separate include.

} // namespace util
