// moved from tests/utils.cpp
#include "utils.h"
#include <random>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <cmath>
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

// --- migrated from util/verify.cpp ---
namespace util {

// Use centralized path helpers
#include "util/path.h"

void print_diffs(const std::vector<int32_t>& a, const std::vector<int32_t>& b) {
    size_t n = std::min(a.size(), b.size());
    size_t diffs = 0;
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            if (diffs < 20) {
                std::cerr << "diff at " << i << ": expected=" << b[i] << " got=" << a[i] << "\n";
            }
            ++diffs;
        }
    }
    if (a.size() != b.size()) {
        std::cerr << "size mismatch: got=" << a.size() << " golden=" << b.size() << "\n";
    }
    std::cerr << "total differences: " << diffs << "\n";
}

bool write_and_compare(const CaseConfig &cfg, const std::vector<AccType> &C,
                       const std::vector<DataType> &A, const std::vector<DataType> &B) {
    std::string c_out_resolved = resolve_path(cfg.c_out_path);
    if (!write_bin_int32_from_acc(c_out_resolved, C)) {
        std::cerr << "write_and_compare: failed to write C_out to " << c_out_resolved << std::endl;
    }

    if (!cfg.c_golden_path.empty()) {
        std::string c_golden_resolved = resolve_path(cfg.c_golden_path);
        std::vector<int32_t> Cgold;
        if (read_bin<int32_t>(c_golden_resolved, Cgold)) {
            if (Cgold.size() != C.size()) {
                std::cerr << "write_and_compare: golden size mismatch: " << Cgold.size() << " vs " << C.size() << std::endl;
                return false;
            }
            bool match = true;
            for (size_t i = 0; i < C.size(); ++i) {
                if (C[i] != Cgold[i]) { match = false; break; }
            }
            if (!match) {
                std::cerr << "write_and_compare: result does not match golden for case " << cfg.case_path << std::endl;
                std::vector<size_t> diffs_idx;
                for (size_t ii = 0; ii < C.size(); ++ii) {
                    if (C[ii] != Cgold[ii]) diffs_idx.push_back(ii);
                }
                size_t show = std::min<size_t>(diffs_idx.size(), 10);
                for (size_t d = 0; d < show; ++d) {
                    size_t idx = diffs_idx[d];
                    size_t row = cfg.M > 0 ? idx / cfg.N : 0;
                    size_t col = cfg.N > 0 ? idx % cfg.N : 0;
                    std::cerr << "diff[" << d << "] idx=" << idx << " (r=" << row << ",c=" << col << ") expected=" << Cgold[idx] << " got=" << C[idx] << "\n";
                    std::cerr << "  A[row] = [";
                    for (int k = 0; k < cfg.K; ++k) {
                        if (k) std::cerr << ",";
                        std::cerr << A[row * cfg.K + k];
                    }
                    std::cerr << "]\n";
                    std::cerr << "  B[col] = [";
                    for (int k = 0; k < cfg.K; ++k) {
                        if (k) std::cerr << ",";
                        std::cerr << B[k * cfg.N + col];
                    }
                    std::cerr << "]\n";
                }

                // Write diffs into a generated-results directory to avoid committing them
                std::filesystem::path outdir = std::filesystem::path(PROJECT_SRC_DIR) / "tests" / "results";
                std::error_code ec;
                std::filesystem::create_directories(outdir, ec);
                std::string base_name;
                if (!cfg.case_path.empty()) {
                    std::filesystem::path cp(cfg.case_path);
                    base_name = cp.stem().string();
                } else {
                    std::filesystem::path cp(cfg.c_out_path);
                    base_name = cp.stem().string();
                }
                std::filesystem::path diff_path = outdir / (base_name + std::string(".diff"));
                std::ofstream df(diff_path);
                if (df) {
                    df << "case=" << cfg.case_path << "\n";
                    df << "total_diffs=" << diffs_idx.size() << "\n";
                    for (size_t idx : diffs_idx) {
                        size_t row = cfg.M > 0 ? idx / cfg.N : 0;
                        size_t col = cfg.N > 0 ? idx % cfg.N : 0;
                        df << idx << "," << row << "," << col << ",got=" << C[idx] << ",exp=" << Cgold[idx] << "\n";
                        df << "A_row:";
                        for (int k = 0; k < cfg.K; ++k) { if (k) df << ","; df << A[row * cfg.K + k]; }
                        df << "\nB_col:";
                        for (int k = 0; k < cfg.K; ++k) { if (k) df << ","; df << B[k * cfg.N + col]; }
                        df << "\n---\n";
                    }
                    df.close();
                    std::cerr << "write_and_compare: wrote diff file: " << diff_path.string() << std::endl;
                }

                print_diffs(std::vector<int32_t>(C.begin(), C.end()), Cgold);
                return false;
            }
        } else {
            std::cerr << "write_and_compare: could not read golden file " << cfg.c_golden_path << std::endl;
        }
    }
    return true;
}

bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                   const std::vector<DataType>& B, int B_rows, int B_cols,
                   const std::vector<AccType>& C) {
    if (A_cols != B_rows) return false;

    int M = A_rows;
    int N = B_cols;
    int K = A_cols;

    if (M <= 0 || N <= 0 || K <= 0) return false;
    if ((int)A.size() != M * K) return false;
    if ((int)B.size() != K * N) return false;
    if ((int)C.size() != M * N) return false;
    
    std::vector<AccType> ref(M * N, 0);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            AccType sum = 0;
            for (int k = 0; k < K; k++) {
                sum += static_cast<AccType>(A[i * K + k]) * 
                       static_cast<AccType>(B[k * N + j]);
            }
            ref[i * N + j] = sum;
        }
    }
    
    double max_error = 0;
    double total_error = 0;
    int errors = 0;
    
    for (int i = 0; i < M * N; i++) {
        double error = std::abs(static_cast<double>(C[i]) - static_cast<double>(ref[i]));
        if (error > 1e-3) {
            errors++;
            std::cout << "Error at (" << i/N << "," << i%N << "): "
                      << C[i] << " vs " << ref[i]
                      << " diff=" << error << std::endl;
        }
        max_error = std::max(max_error, error);
        total_error += error;
    }
    
    if (errors == 0) {
        std::cout << "Verification PASSED! Max error: " << max_error 
                  << ", Avg error: " << total_error/(M*N) << std::endl;
        return true;
    } else {
        std::cout << "Verification FAILED! " << errors << " errors found." << std::endl;
        return false;
    }
}

} // namespace util

