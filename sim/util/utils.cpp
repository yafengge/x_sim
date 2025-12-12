// moved from tests/utils.cpp
#include "utils.h"
#include <random>
#include <fstream>
#include <sstream>
// removed sys/stat.h usage (file existence helper removed)
namespace util {

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

bool write_matrix_bin(const std::string& path, const std::vector<int16_t>& data) {
    std::ofstream fout(path, std::ios::binary);
    if (!fout) return false;
    fout.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(int16_t));
    return fout.good();
}

bool read_matrix_bin(const std::string& path, std::vector<int16_t>& out) {
    std::ifstream fin(path, std::ios::binary | std::ios::ate);
    if (!fin) return false;
    auto size = fin.tellg();
    if (size <= 0) return false;
    fin.seekg(0);
    out.resize(size / sizeof(int16_t));
    fin.read(reinterpret_cast<char*>(out.data()), size);
    return fin.good();
}

} // namespace util

