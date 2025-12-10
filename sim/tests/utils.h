#pragma once
#include <vector>
#include <string>

// Helpers used by tests: write small TOML config files and generate random matrices.
std::vector<int16_t> generate_random_matrix(int rows, int cols, int min_val = -128, int max_val = 127);

void write_config_file(const std::string& path, int array_rows, int array_cols);

// Optionally add more helpers (print_matrix, etc.) later.
