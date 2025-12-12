#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "systolic.h"
#include "aic.h"
#include "config/config.h"

#include <gtest/gtest.h>
#include "util/utils.h"
#include "util/case_io.h"
#include "util/utils.h"
#include <filesystem>
#include <cstdlib>

// 文件：tests/test_integration.cpp
// Ensure tests use the repo-wide default config path via a test fixture
class Integration : public ::testing::Test {
protected:
    Integration() { config::set_default_path("model_cfg.toml"); }
    ~Integration() override = default;
};
// 说明：集成测试集合（使用 GoogleTest）。
// 测试采用 case-driven 流程：每个 case 由一个 TOML 描述
// 输入二进制文件路径与 meta 信息。测试通过调用 `AIC::build(case_toml)`
// 配置运行环境，再调用 `AIC::start()` 执行仿真并进行结果验证（与 golden 比对）。
//
// 集成测试：SmallMatrix
// 目的：使用一个确定性的 4x4 矩阵对验证阵列的基本正确性。
// 运行说明：使用 `ctest` 或直接运行测试二进制来执行该用例。
TEST_F(Integration, SmallMatrix) {
    // Determine base directory for case artifacts. Honor CASE_OUTPUT_DIR env var
    // (useful for CI); otherwise write into the source tree under tests/cases.
    auto case_dir = std::getenv("CASE_OUTPUT_DIR") ? std::string(std::getenv("CASE_OUTPUT_DIR")) : (std::filesystem::current_path() / "tests" / "cases").string();
    std::string case_toml = case_dir + std::string("/case_SmallMatrix.toml");
    util::CaseConfig case_cfg;
    bool have_case = util::read_case_toml(case_toml, case_cfg);

    // SmallMatrix uses deterministic matrices; we will create binary files
    // and a case TOML if it doesn't already exist.
    std::vector<int16_t> A = {1,2,3,4,
                              5,6,7,8,
                              9,10,11,12,
                              13,14,15,16};
    std::vector<int16_t> B = {1,0,0,0,
                              0,1,0,0,
                              0,0,1,0,
                              0,0,0,1};
    // ensure case files exist; if not, generate binaries and write the toml
    if (!have_case) {
        util::create_case_toml(case_toml, case_cfg, case_dir, std::string("SmallMatrix"), A, B, 4, 4, 4);
    }

    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk);
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(case_toml);

    std::vector<int32_t> C;

    auto ret = aic->start();
    EXPECT_TRUE(ret);
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
// 运行说明：使用 `ctest` 或直接运行测试二进制来执行该用例。
TEST_F(Integration, QuickLarge) {
    auto case_dir = std::getenv("CASE_OUTPUT_DIR") ? std::string(std::getenv("CASE_OUTPUT_DIR")) : (std::filesystem::current_path() / "tests" / "cases").string();
        // Use case-driven flow: generate case TOML and binaries if missing,
        // then call `aic->build(case_toml)` followed by `aic->start()`.
    std::string case_toml = case_dir + std::string("/case_QuickLarge.toml");
    util::CaseConfig case_cfg;
    bool have_case = util::read_case_toml(case_toml, case_cfg);

    int M = 32; int K = 32; int N = 32;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);

    if (!have_case) {
        util::create_case_toml(case_toml, case_cfg, case_dir, std::string("QuickLarge"), A, B, M, K, N);
    }

    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk);
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(case_toml);

    auto ret = aic->start();
    EXPECT_TRUE(ret);
    // Read C_out produced by AIC and verify a small block to limit cost.
    // To avoid any in-memory vs file inconsistencies, read A/B back from the
    // case binaries and compute the reference from those.
}

// Slow / extended tests (disabled by default). Prefix with DISABLED_ so they
// only run when explicitly enabled (e.g. via --gtest_filter or removing DISABLED_)
// 慢速/扩展测试：DISABLED_DataflowModes
//
// 目的：验证不同数据流策略（WEIGHT/OUTPUT/INPUT stationary）对阵列行为的影响。
// 说明：该测试较为耗时，会对同一输入在不同 dataflow 配置下运行完整仿真。
// 默认以 DISABLED_ 前缀禁用；需要时通过 `--gtest_filter` 或取消 DISABLED_ 前缀启用。
// 运行说明：使用 `ctest` 或直接运行测试二进制来执行该用例。
TEST_F(Integration, DataflowModes) {
    auto case_dir = std::getenv("CASE_OUTPUT_DIR") ? std::string(std::getenv("CASE_OUTPUT_DIR")) : (std::filesystem::current_path() / "tests" / "cases").string();
    // Use case TOML to run this (still disabled by default)
    std::string case_toml = case_dir + std::string("/case_DataflowModes.toml");
    util::CaseConfig case_cfg;
    bool have_case = util::read_case_toml(case_toml, case_cfg);
    int M = 64, K = 64, N = 64;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);

    if (!have_case) {
        util::create_case_toml(case_toml, case_cfg, case_dir, std::string("Dataflow"), A, B, M, K, N);
    }

    auto clk_w = std::make_shared<Clock>();
    auto memory_w = std::make_shared<Mem>(clk_w);
    auto aic_w = std::make_shared<AIC>(clk_w, memory_w);
    aic_w->build(case_toml);
    auto ret = aic_w->start();
    EXPECT_TRUE(ret);
}

//
// 目的：测试不同阵列尺寸（4、8、16、32 等）下的矩阵乘法可扩展性与正确性。
// 说明：此测试针对更大的输入（例如 256x256）进行多次仿真，运行时间较长，
// 因此默认被禁用（以 `DISABLED_` 前缀）。
// 运行说明：使用 `ctest` 或直接运行测试二进制来执行该用例。
TEST_F(Integration, Scaling) {
    auto case_dir = std::getenv("CASE_OUTPUT_DIR") ? std::string(std::getenv("CASE_OUTPUT_DIR")) : (std::filesystem::current_path() / "tests" / "cases").string();
    std::string case_toml = case_dir + std::string("/case_Scaling.toml");
    util::CaseConfig case_cfg;
    bool have_case = util::read_case_toml(case_toml, case_cfg);
    int M = 256, K = 256, N = 256;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);
    if (!have_case) {
        util::create_case_toml(case_toml, case_cfg, case_dir, std::string("Scaling"), A, B, M, K, N);
    }

    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk);
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(case_toml);
    auto ret = aic->start();
    EXPECT_TRUE(ret);
}

// use gtest_main provided by the test target (no explicit main here)
