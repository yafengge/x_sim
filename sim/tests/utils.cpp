#include "utils.h"
#include <random>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

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

static bool file_exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool write_case_toml(const std::string& path, const CaseMeta& meta) {
    std::ofstream fout(path);
    if (!fout) return false;
    fout << "[a]\n";
    fout << "path = \"" << meta.a_path << "\"\n";
    fout << "rows = " << meta.a_rows << "\n";
    fout << "cols = " << meta.a_cols << "\n";
    fout << "[b]\n";
    fout << "path = \"" << meta.b_path << "\"\n";
    fout << "rows = " << meta.b_rows << "\n";
    fout << "cols = " << meta.b_cols << "\n";
    return fout.good();
}

bool read_case_toml(const std::string& path, CaseMeta& meta) {
    if (!file_exists(path)) return false;
    std::ifstream fin(path);
    if (!fin) return false;
    std::string line;
    std::string section;
    while (std::getline(fin, line)) {
        // trim leading spaces
        size_t p = line.find_first_not_of(" \t\r\n");
        if (p == std::string::npos) continue;
        line = line.substr(p);
        if (line.empty() || line[0] == '#') continue;
        if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size()-2);
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq+1);
        // trim
        auto trim = [](std::string &s){
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            if (a==std::string::npos) { s.clear(); return; }
            s = s.substr(a, b-a+1);
        };
        trim(key); trim(val);
        // strip optional quotes
        if (!val.empty() && val.front()=='\"' && val.back()=='\"') val = val.substr(1, val.size()-2);
        try {
            if (section == "a") {
                if (key == "path") meta.a_path = val;
                else if (key == "rows") meta.a_rows = std::stoi(val);
                else if (key == "cols") meta.a_cols = std::stoi(val);
            } else if (section == "b") {
                if (key == "path") meta.b_path = val;
                else if (key == "rows") meta.b_rows = std::stoi(val);
                else if (key == "cols") meta.b_cols = std::stoi(val);
            }
        } catch (...) {
            // ignore parse errors, treat as failure
            return false;
        }
    }
    return true;
}
