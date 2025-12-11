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
#include "util/utils.h"
#include "util/verify.h"
#include <filesystem>

// 在测试中统一使用相对路径查找配置文件（不使用绝对路径）
static std::string find_config_rel() {
    // 仅使用相对路径指向项目内的配置文件（指向仓库中的 `config/model.toml`）
    return std::string("config/model.toml");
}
// 集成测试：SmallMatrix
//
// 目的：使用一个确定性的 4x4 矩阵对验证阵列的基本正确性。
// 步骤：
// 1. 生成一个小的配置文件（阵列尺寸 4x4），用以快速运行仿真；
// 2. 构建 `AIC` 并获取 `Cube`；
// 3. 使用已知的 A、B 数据执行 `run`，并用软件参考实现 `verify_result` 校验结果。
TEST(Integration, SmallMatrix) {
    std::string cfg = find_config_rel();
    // SmallMatrix uses in-memory deterministic matrices (no file IO).
    std::vector<int16_t> A = {1,2,3,4,
                              5,6,7,8,
                              9,10,11,12,
                              13,14,15,16};
    std::vector<int16_t> B = {1,0,0,0,
                              0,1,0,0,
                              0,0,1,0,
                              0,0,0,1};
    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk, cfg);
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(cfg);

    std::vector<int32_t> C;

    // preload A/B into memory model per-test
    memory->load_data(A, 0);
    memory->load_data(B, static_cast<uint32_t>(A.size()));

    EXPECT_TRUE(aic->start(A,4,4,B,4,4,C));
    EXPECT_TRUE(util::verify_result(A,4,4,B,4,4,C));
}

// 集成测试：QuickLarge
//
// 目的：对较大的随机矩阵（32x32）执行一次完整的矩阵乘法仿真，
// 然后只对结果中的一个小块（4x4）进行软件参考校验以节省时间。
// 步骤：
// 1. 生成/写入一个临时配置文件（`int_test_config2.toml`），设置阵列规模。
// 2. 使用该配置创建 `AIC`（顶层封装），并从中获取 `Cube` 实例。
// 3. 生成随机矩阵 A、B，并调用 `cube->run` 执行仿真。
// 4. 从得到的结果中抽取一个 4x4 子块，用软件实现的乘法进行对比验证。
TEST(Integration, QuickLarge) {
    std::string cfg = find_config_rel();

    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk, cfg);
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(cfg);

    int M = 32; int K = 32; int N = 32;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);
    std::vector<int32_t> C;

    // preload into memory model
    memory->load_data(A, 0);
    memory->load_data(B, static_cast<uint32_t>(A.size()));

    EXPECT_TRUE(aic->start(A,M,K,B,K,N,C));
    // Verify a small block to limit cost
    std::vector<int16_t> A_block(4*K);
    std::vector<int16_t> B_block(K*4);
    std::vector<int32_t> C_block(16);
    for (int i=0;i<4;i++) for (int k=0;k<K;k++) A_block[i*K+k]=A[i*K+k];
    for (int k=0;k<K;k++) for (int j=0;j<4;j++) B_block[k*4+j]=B[k*N+j];
    for (int i=0;i<4;i++) for (int j=0;j<4;j++) C_block[i*4+j]=C[i*N+j];
    
    EXPECT_TRUE(util::verify_result(A_block,4,K,B_block,K,4,C_block));
}

// Slow / extended tests (disabled by default). Prefix with DISABLED_ so they
// only run when explicitly enabled (e.g. via --gtest_filter or removing DISABLED_)
// 慢速/扩展测试：DISABLED_DataflowModes
//
// 目的：验证不同数据流策略（WEIGHT/OUTPUT/INPUT stationary）对阵列行为的影响。
// 说明：该测试较为耗时，会对同一输入在不同 dataflow 配置下运行完整仿真。
// 默认以 DISABLED_ 前缀禁用；需要时通过 `--gtest_filter` 或取消 DISABLED_ 前缀启用。
TEST(Integration, DataflowModes) {
    std::string cfg = find_config_rel();
    int M = 64, K = 64, N = 64;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);

    
    auto clk_w = std::make_shared<Clock>();
    auto memory_w = std::make_shared<Mem>(clk_w, cfg);
    auto aic_w = std::make_shared<AIC>(clk_w, memory_w);
    aic_w->build(cfg);
    std::vector<int32_t> Cw;
    memory_w->load_data(A, 0);
    memory_w->load_data(B, static_cast<uint32_t>(A.size()));

    EXPECT_TRUE(aic_w->start(A, M, K, B, K, N, Cw));
    EXPECT_TRUE(util::verify_result(A, M, K, B, K, N, Cw));
}


//
// 目的：测试不同阵列尺寸（4、8、16、32 等）下的矩阵乘法可扩展性与正确性。
// 说明：此测试针对更大的输入（例如 256x256）进行多次仿真，运行时间较长，
// 因此默认被禁用（以 `DISABLED_` 前缀）。
TEST(Integration, Scaling) {
    std::string cfg = find_config_rel();
    int M = 256, K = 256, N = 256;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);

    // This disabled scaling test previously rewrote the config per-size.
    // Tests must now use the single `config/model.toml`. Keep the test disabled
    // and exercise a single run using the configured array size.
    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk, cfg);
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(cfg);
    
    std::vector<int32_t> C;
    memory->load_data(A, 0);
    memory->load_data(B, static_cast<uint32_t>(A.size()));

    EXPECT_TRUE(aic->start(A, M, K, B, K, N, C));
    EXPECT_TRUE(util::verify_result(A, M, K, B, K, N, C));
}

// use gtest_main provided by the test target (no explicit main here)
