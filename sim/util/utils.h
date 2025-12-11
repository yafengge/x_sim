#pragma once
#include <vector>
#include <string>

// Helpers moved from tests/utils.* to util/ to be shared by tests and tools.
std::vector<int16_t> generate_random_matrix(int rows, int cols, int min_val = -128, int max_val = 127);

void write_config_file(const std::string& path, int array_rows, int array_cols);

// Save/load matrix in raw binary form (row-major int16_t). Returns true on success.
bool write_matrix_bin(const std::string& path, const std::vector<int16_t>& data);
bool read_matrix_bin(const std::string& path, std::vector<int16_t>& out);

// Per-case TOML metadata used by tests. Simple writer/reader sufficient for tests.
struct CaseMeta {
    std::string a_path;
    int a_rows = 0;
    int a_cols = 0;
    std::string b_path;
    int b_rows = 0;
    int b_cols = 0;
};

bool write_case_toml(const std::string& path, const CaseMeta& meta);
bool read_case_toml(const std::string& path, CaseMeta& meta);
