#include "utils.h"
#include <random>
#include <fstream>

std::vector<int16_t> generate_random_matrix(int rows, int cols, int min_val, int max_val) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min_val, max_val);
    std::vector<int16_t> matrix(rows * cols);
    for (int i = 0; i < rows * cols; ++i) matrix[i] = static_cast<int16_t>(dis(gen));
    return matrix;
}

void write_config_file(const std::string& path, int array_rows, int array_cols) {
    std::ofstream fout(path);
    fout << "[cube]\n";
    fout << "array_rows = " << array_rows << "\n";
    fout << "array_cols = " << array_cols << "\n";
}
