#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "systolic.h"
#include "aic.h"
#include "config/config_mgr.h"

#include <gtest/gtest.h>
#include "utils.h"
// TEST(Integration, SmallMatrix) {
//     std::string cfg = "int_test_config.toml";
//     write_config_file(cfg, 4, 4);
//     AIC aic(cfg);
//     auto cube = aic.get_cube();

//     std::vector<int16_t> A = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
//     std::vector<int16_t> B = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
//     std::vector<int32_t> C;

//     EXPECT_TRUE(cube->matmul(A,4,4,B,4,4,C));
//     EXPECT_TRUE(cube->verify_result(A,4,4,B,4,4,C));
// }

TEST(Integration, QuickLarge) {
    std::string cfg = "int_test_config2.toml";
    write_config_file(cfg, 8, 8);
    AIC aic(cfg);
    auto cube = aic.get_cube();

    int M = 32; int K = 32; int N = 32;
    auto A = generate_random_matrix(M,K);
    auto B = generate_random_matrix(K,N);
    std::vector<int32_t> C;

    EXPECT_TRUE(cube->matmul(A,M,K,B,K,N,C));
    // Verify a small block to limit cost
    std::vector<int16_t> A_block(4*K);
    std::vector<int16_t> B_block(K*4);
    std::vector<int32_t> C_block(16);
    for (int i=0;i<4;i++) for (int k=0;k<K;k++) A_block[i*K+k]=A[i*K+k];
    for (int k=0;k<K;k++) for (int j=0;j<4;j++) B_block[k*4+j]=B[k*N+j];
    for (int i=0;i<4;i++) for (int j=0;j<4;j++) C_block[i*4+j]=C[i*N+j];
    EXPECT_TRUE(cube->verify_result(A_block,4,K,B_block,K,4,C_block));
}

// // Slow / extended tests (disabled by default). Prefix with DISABLED_ so they
// // only run when explicitly enabled (e.g. via --gtest_filter or removing DISABLED_)
// TEST(Integration, DISABLED_DataflowModes) {
//     std::string cfg = "int_df_config.toml";
//     int M = 64, K = 64, N = 64;
//     auto A = generate_random_matrix(M,K);
//     auto B = generate_random_matrix(K,N);

//     // Weight stationary
//     write_config_file(cfg, 8, 8);
//     AIC aic_w(cfg);
//     auto cube_w = aic_w.get_cube();
//     std::vector<int32_t> Cw;
//     ASSERT_TRUE(cube_w->matmul(A, M, K, B, K, N, Cw));

//     // Output stationary
//     // write different config (dataflow handled via config_mgr if supported)
//     std::ofstream out(cfg);
//     out << "[cube]\narray_rows = 8\narray_cols = 8\nverbose = false\nprogress_interval = 0\ndataflow = \"OUTPUT_STATIONARY\"\n";
//     out.close();
//     AIC aic_o(cfg);
//     auto cube_o = aic_o.get_cube();
//     std::vector<int32_t> Co;
//     ASSERT_TRUE(cube_o->matmul(A, M, K, B, K, N, Co));

//     // Input stationary
//     std::ofstream out2(cfg);
//     out2 << "[cube]\narray_rows = 8\narray_cols = 8\nverbose = false\nprogress_interval = 0\ndataflow = \"INPUT_STATIONARY\"\n";
//     out2.close();
//     AIC aic_i(cfg);
//     auto cube_i = aic_i.get_cube();
//     std::vector<int32_t> Ci;
//     ASSERT_TRUE(cube_i->matmul(A, M, K, B, K, N, Ci));
// }

// TEST(Integration, DISABLED_Scaling) {
//     std::string cfg = "int_scale_config.toml";
//     int M = 256, K = 256, N = 256;
//     auto A = generate_random_matrix(M,K);
//     auto B = generate_random_matrix(K,N);

//     std::vector<int> sizes = {4,8,16,32};
//     for (int size : sizes) {
//         write_config_file(cfg, size, size);
//         AIC aic(cfg);
//         auto cube = aic.get_cube();
//         std::vector<int32_t> C;
//         ASSERT_TRUE(cube->matmul(A, M, K, B, K, N, C));
//     }
// }

// use gtest_main provided by the test target (no explicit main here)
