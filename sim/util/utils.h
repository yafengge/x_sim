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

// Verification utilities (migrated from util/verify.h)
// Resolve a possibly-relative path using PROJECT_SRC_DIR when needed
std::string resolve_path(const std::string &path);

// Print simple diffs between two int32 vectors (used by write_and_compare)
void print_diffs(const std::vector<int32_t>& a, const std::vector<int32_t>& b);

// Write C (accumulators) to disk and compare with golden described in CaseConfig.
// Returns true if results match or no golden provided; false if mismatch or IO error.
bool write_and_compare(const CaseConfig &cfg, const std::vector<AccType> &C,
					   const std::vector<DataType> &A, const std::vector<DataType> &B);

// Software verification helper: compute reference and compare with C
bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
				   const std::vector<DataType>& B, int B_rows, int B_cols,
				   const std::vector<AccType>& C);

} // namespace util
