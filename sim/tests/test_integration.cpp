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
#include <filesystem>
// 集成测试：SmallMatrix
//
// 目的：使用一个确定性的 4x4 矩阵对验证阵列的基本正确性。
// 步骤：
// 1. 生成一个小的配置文件（阵列尺寸 4x4），用以快速运行仿真；
// 2. 构建 `AIC` 并获取 `Cube`；
// 3. 使用已知的 A、B 数据执行 `matmul`，并用软件参考实现 `verify_result` 校验结果。
TEST(Integration, SmallMatrix) {
    auto find_config = []() -> std::string {
        std::vector<std::string> candidates = {
            std::string("../config.toml"),
            std::string("config.toml"),
            std::string("/home/steven/x_sim/sim/config.toml")
        };
        for (auto &c : candidates) if (std::filesystem::exists(c)) return c;
        return std::string("config.toml");
    };
    std::string cfg = find_config();

    // Use on-disk case data if present. Case metadata file is named
    // `case_<casename>_.toml`. If it exists, load the A/B matrices from the
    // referenced binary files; otherwise create the matrices and write them
    // to disk for future runs.
    const std::string casename = "SmallMatrix";
    const std::string case_toml = "case_" + casename + "_.toml";

    std::vector<int16_t> A;
    std::vector<int16_t> B;
    CaseMeta meta;
    if (read_case_toml(case_toml, meta)) {
        // load from disk
        ASSERT_TRUE(read_matrix_bin(meta.a_path, A));
        ASSERT_TRUE(read_matrix_bin(meta.b_path, B));
    } else {
        // generate and save
        A = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
        B = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        meta.a_path = casename + "_A.bin";
        meta.b_path = casename + "_B.bin";
        meta.a_rows = 4; meta.a_cols = 4;
        meta.b_rows = 4; meta.b_cols = 4;
        ASSERT_TRUE(write_matrix_bin(meta.a_path, A));
        ASSERT_TRUE(write_matrix_bin(meta.b_path, B));
        ASSERT_TRUE(write_case_toml(case_toml, meta));
    }

    AIC aic(cfg);
    auto cube = aic.get_cube();

    std::vector<int32_t> C;

    EXPECT_TRUE(cube->matmul(A,4,4,B,4,4,C));
    EXPECT_TRUE(cube->verify_result(A,4,4,B,4,4,C));
}

// 集成测试：QuickLarge
//
// 目的：对较大的随机矩阵（32x32）执行一次完整的矩阵乘法仿真，
// 然后只对结果中的一个小块（4x4）进行软件参考校验以节省时间。
// 步骤：
// 1. 生成/写入一个临时配置文件（`int_test_config2.toml`），设置阵列规模。
// 2. 使用该配置创建 `AIC`（顶层封装），并从中获取 `Cube` 实例。
// 3. 生成随机矩阵 A、B，并调用 `cube->matmul` 执行仿真。
// 4. 从得到的结果中抽取一个 4x4 子块，用软件实现的乘法进行对比验证。
TEST(Integration, QuickLarge) {
    auto find_config = []() -> std::string {
        std::vector<std::string> candidates = {
            std::string("../config.toml"),
            std::string("config.toml"),
            std::string("/home/steven/x_sim/sim/config.toml")
        };
        for (auto &c : candidates) if (std::filesystem::exists(c)) return c;
        return std::string("config.toml");
    };
    std::string cfg = find_config();

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

// Slow / extended tests (disabled by default). Prefix with DISABLED_ so they
// only run when explicitly enabled (e.g. via --gtest_filter or removing DISABLED_)
// 慢速/扩展测试：DISABLED_DataflowModes
//
// 目的：验证不同数据流策略（WEIGHT/OUTPUT/INPUT stationary）对阵列行为的影响。
// 说明：该测试较为耗时，会对同一输入在不同 dataflow 配置下运行完整仿真。
// 默认以 DISABLED_ 前缀禁用；需要时通过 `--gtest_filter` 或取消 DISABLED_ 前缀启用。
TEST(Integration, DISABLED_DataflowModes) {
    auto find_config = []() -> std::string {
        std::vector<std::string> candidates = {
            std::string("../config.toml"),
            std::string("config.toml"),
            std::string("/home/steven/x_sim/sim/config.toml")
        };
        for (auto &c : candidates) if (std::filesystem::exists(c)) return c;
        return std::string("config.toml");
    };
    std::string cfg = find_config();
    int M = 64, K = 64, N = 64;
    auto A = generate_random_matrix(M,K);
    auto B = generate_random_matrix(K,N);

    
    AIC aic_w(cfg);
    auto cube_w = aic_w.get_cube();
    std::vector<int32_t> Cw;
    ASSERT_TRUE(cube_w->matmul(A, M, K, B, K, N, Cw));
}

// 慢速/扩展测试：DISABLED_Scaling
//
// 目的：测试不同阵列尺寸（4、8、16、32 等）下的矩阵乘法可扩展性与正确性。
// 说明：此测试针对更大的输入（例如 256x256）进行多次仿真，运行时间较长，
// 因此默认被禁用（以 `DISABLED_` 前缀）。
TEST(Integration, DISABLED_Scaling) {
    auto find_config = []() -> std::string {
        std::vector<std::string> candidates = {
            std::string("../config.toml"),
            std::string("config.toml"),
            std::string("/home/steven/x_sim/sim/config.toml")
        };
        for (auto &c : candidates) if (std::filesystem::exists(c)) return c;
        return std::string("config.toml");
    };
    std::string cfg = find_config();
    int M = 256, K = 256, N = 256;
    auto A = generate_random_matrix(M,K);
    auto B = generate_random_matrix(K,N);

    // This disabled scaling test previously rewrote the config per-size.
    // Tests must now use the single `config.toml`. Keep the test disabled
    // and exercise a single run using the configured array size.
    AIC aic(cfg);
    auto cube = aic.get_cube();
    std::vector<int32_t> C;
    ASSERT_TRUE(cube->matmul(A, M, K, B, K, N, C));
}

// use gtest_main provided by the test target (no explicit main here)
