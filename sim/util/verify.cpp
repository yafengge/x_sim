#include "util/verify.h"

namespace util {

bool verify_result(const std::vector<DataType>& A, int A_rows, int A_cols,
                   const std::vector<DataType>& B, int B_rows, int B_cols,
                   const std::vector<AccType>& C) {
    if (A_cols != B_rows) return false;

    int M = A_rows;
    int N = B_cols;
    int K = A_cols;

    // 参数合法性检查：尺寸必须为正，且输入向量长度匹配
    if (M <= 0 || N <= 0 || K <= 0) return false;
    if ((int)A.size() != M * K) return false;
    if ((int)B.size() != K * N) return false;
    if ((int)C.size() != M * N) return false;
    
    // 软件计算参考结果
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
    
    // 比较结果
    double max_error = 0;
    double total_error = 0;
    int errors = 0;
    
    for (int i = 0; i < M * N; i++) {
        double error = std::abs(static_cast<double>(C[i]) - static_cast<double>(ref[i]));
        if (error > 1e-3) {  // 容差
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
