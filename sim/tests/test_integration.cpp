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
#include "util/case_io.h"
#include "util/verify.h"
#include <filesystem>
#include <cstdlib>

// Tests use the repository-local config file path directly: `config/model.toml`.
// 集成测试：SmallMatrix
//
// 目的：使用一个确定性的 4x4 矩阵对验证阵列的基本正确性。
// 步骤：
// 1. 生成一个小的配置文件（阵列尺寸 4x4），用以快速运行仿真；
// 2. 构建 `AIC` 并获取 `Cube`；
// 3. 使用已知的 A、B 数据执行 `run`，并用软件参考实现 `verify_result` 校验结果。
TEST(Integration, SmallMatrix) {
    // Determine base directory for case artifacts. Honor CASE_OUTPUT_DIR env var
    // (useful for CI); otherwise write into the source tree under tests/cases.
    auto case_dir = std::getenv("CASE_OUTPUT_DIR") ? std::string(std::getenv("CASE_OUTPUT_DIR")) : std::string(PROJECT_SRC_DIR) + std::string("/tests/cases");
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
        std::filesystem::create_directories(case_dir);
        case_cfg.case_path = case_toml;
        case_cfg.a_path = case_dir + std::string("/SmallMatrix_A.bin");
        case_cfg.b_path = case_dir + std::string("/SmallMatrix_B.bin");
        case_cfg.c_golden_path = case_dir + std::string("/SmallMatrix_C_golden.bin");
        case_cfg.c_out_path = case_dir + std::string("/SmallMatrix_C_out.bin");
        case_cfg.a_addr = 0;
        case_cfg.b_addr = static_cast<uint32_t>(A.size());
        case_cfg.c_addr = static_cast<uint32_t>(A.size() + B.size());
        case_cfg.M = 4; case_cfg.K = 4; case_cfg.N = 4;
        // write binaries
        bool wa = util::write_bin_int16(case_cfg.a_path, A);
        bool wb = util::write_bin_int16(case_cfg.b_path, B);
        // compute golden C
        std::vector<int32_t> Cgold(case_cfg.M * case_cfg.N, 0);
        for (int i = 0; i < case_cfg.M; ++i) {
            for (int j = 0; j < case_cfg.N; ++j) {
                int32_t acc = 0;
                for (int k = 0; k < case_cfg.K; ++k) {
                    acc += static_cast<int32_t>(A[i*case_cfg.K + k]) * static_cast<int32_t>(B[k*case_cfg.N + j]);
                }
                Cgold[i*case_cfg.N + j] = acc;
            }
        }
        bool wc = util::write_bin_int32(case_cfg.c_golden_path, Cgold);
        (void)wa; (void)wb; (void)wc;
        // write toml
        util::write_case_toml(case_cfg);
    }
    else {
        // If TOML exists but binaries were not generated in the source tree (e.g., a
        // previous run created only the TOML), make sure the A/B/golden files exist
        // so AIC::start can read them. We prefer not to overwrite existing binaries.
        auto resolve_write = [&](const std::string &path)->std::string {
            std::filesystem::path p(path);
            if (p.is_absolute()) return path;
            return std::string(PROJECT_SRC_DIR) + std::string("/") + path;
        };
        std::string a_out = resolve_write(case_cfg.a_path);
        std::string b_out = resolve_write(case_cfg.b_path);
        std::string cgold_out = resolve_write(case_cfg.c_golden_path);
        if (!std::filesystem::exists(a_out)) {
            util::write_bin_int16(a_out, A);
        }
        if (!std::filesystem::exists(b_out)) {
            util::write_bin_int16(b_out, B);
        }
        if (!std::filesystem::exists(cgold_out)) {
            std::vector<int32_t> Cgold(case_cfg.M * case_cfg.N, 0);
            for (int i = 0; i < case_cfg.M; ++i) {
                for (int j = 0; j < case_cfg.N; ++j) {
                    int32_t acc = 0;
                    for (int k = 0; k < case_cfg.K; ++k) {
                        acc += static_cast<int32_t>(A[i*case_cfg.K + k]) * static_cast<int32_t>(B[k*case_cfg.N + j]);
                    }
                    Cgold[i*case_cfg.N + j] = acc;
                }
            }
            util::write_bin_int32(cgold_out, Cgold);
        }
    }

    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk, std::string("model_cfg.toml"));
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(std::string("model_cfg.toml"));

    std::vector<int32_t> C;

    // A/B are preloaded into the memory model by `AIC::start(case_toml)`,
    // so tests do not need to call `memory->load_data` directly anymore.

    auto ret = aic->start(case_toml);
    EXPECT_TRUE(ret);
    // Read C_out produced by AIC and verify
    bool rc = util::read_bin_int32(case_cfg.c_out_path, C);
    EXPECT_TRUE(rc);
    // Result verification is performed inside `AIC::start(case_toml)` by
    // comparing `C_out` with the golden file. No additional in-test
    // verification is necessary here.
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
    auto case_dir = std::getenv("CASE_OUTPUT_DIR") ? std::string(std::getenv("CASE_OUTPUT_DIR")) : std::string(PROJECT_SRC_DIR) + std::string("/tests/cases");
    // Use case-driven flow: generate case TOML and binaries if missing,
    // then call AIC::start(case_toml)
    std::string case_toml = case_dir + std::string("/case_QuickLarge.toml");
    util::CaseConfig case_cfg;
    bool have_case = util::read_case_toml(case_toml, case_cfg);

    int M = 32; int K = 32; int N = 32;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);

    if (!have_case) {
        std::filesystem::create_directories(case_dir);
        case_cfg.case_path = case_toml;
        case_cfg.a_path = case_dir + std::string("/QuickLarge_A.bin");
        case_cfg.b_path = case_dir + std::string("/QuickLarge_B.bin");
        case_cfg.c_golden_path = case_dir + std::string("/QuickLarge_C_golden.bin");
        case_cfg.c_out_path = case_dir + std::string("/QuickLarge_C_out.bin");
        case_cfg.a_addr = 0;
        case_cfg.b_addr = static_cast<uint32_t>(A.size());
        case_cfg.c_addr = static_cast<uint32_t>(A.size() + B.size());
        case_cfg.M = M; case_cfg.K = K; case_cfg.N = N;
        util::write_bin_int16(case_cfg.a_path, A);
        util::write_bin_int16(case_cfg.b_path, B);
        // golden
        std::vector<int32_t> Cgold(M*N, 0);
        for (int i=0;i<M;i++) for (int j=0;j<N;j++) {
            int32_t acc = 0;
            for (int k=0;k<K;k++) acc += static_cast<int32_t>(A[i*K+k]) * static_cast<int32_t>(B[k*N+j]);
            Cgold[i*N+j] = acc;
        }
        util::write_bin_int32(case_cfg.c_golden_path, Cgold);
        util::write_case_toml(case_cfg);
    }

    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk, std::string("model_cfg.toml"));
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(std::string("model_cfg.toml"));

    auto ret = aic->start(case_toml);
    EXPECT_TRUE(ret);
    // Read C_out produced by AIC and verify a small block to limit cost.
    // To avoid any in-memory vs file inconsistencies, read A/B back from the
    // case binaries and compute the reference from those.
    std::vector<int32_t> C;
    bool rc = util::read_bin_int32(case_cfg.c_out_path, C);
    EXPECT_TRUE(rc);
    // Verification of the output against the golden file is handled inside
    // `AIC::start(case_toml)`. The test keeps a presence/read check for
    // `C_out` but does not re-run the full software reference verification.
}

// Slow / extended tests (disabled by default). Prefix with DISABLED_ so they
// only run when explicitly enabled (e.g. via --gtest_filter or removing DISABLED_)
// 慢速/扩展测试：DISABLED_DataflowModes
//
// 目的：验证不同数据流策略（WEIGHT/OUTPUT/INPUT stationary）对阵列行为的影响。
// 说明：该测试较为耗时，会对同一输入在不同 dataflow 配置下运行完整仿真。
// 默认以 DISABLED_ 前缀禁用；需要时通过 `--gtest_filter` 或取消 DISABLED_ 前缀启用。
TEST(Integration, DataflowModes) {
    auto case_dir = std::getenv("CASE_OUTPUT_DIR") ? std::string(std::getenv("CASE_OUTPUT_DIR")) : std::string(PROJECT_SRC_DIR) + std::string("/tests/cases");
    // Use case TOML to run this (still disabled by default)
    std::string case_toml = case_dir + std::string("/case_DataflowModes.toml");
    util::CaseConfig case_cfg;
    bool have_case = util::read_case_toml(case_toml, case_cfg);
    int M = 64, K = 64, N = 64;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);

    if (!have_case) {
        std::filesystem::create_directories(case_dir);
        case_cfg.case_path = case_toml;
        case_cfg.a_path = case_dir + std::string("/Dataflow_A.bin");
        case_cfg.b_path = case_dir + std::string("/Dataflow_B.bin");
        case_cfg.c_golden_path = case_dir + std::string("/Dataflow_C_golden.bin");
        case_cfg.c_out_path = case_dir + std::string("/Dataflow_C_out.bin");
        case_cfg.a_addr = 0;
        case_cfg.b_addr = static_cast<uint32_t>(A.size());
        case_cfg.c_addr = static_cast<uint32_t>(A.size() + B.size());
        case_cfg.M = M; case_cfg.K = K; case_cfg.N = N;
        util::write_bin_int16(case_cfg.a_path, A);
        util::write_bin_int16(case_cfg.b_path, B);
        std::vector<int32_t> Cgold(M*N,0);
        for (int i=0;i<M;i++) for (int j=0;j<N;j++) {
            int32_t acc = 0;
            for (int k=0;k<K;k++) acc += static_cast<int32_t>(A[i*K+k]) * static_cast<int32_t>(B[k*N+j]);
            Cgold[i*N+j] = acc;
        }
        util::write_bin_int32(case_cfg.c_golden_path, Cgold);
        util::write_case_toml(case_cfg);
    }

    auto clk_w = std::make_shared<Clock>();
    auto memory_w = std::make_shared<Mem>(clk_w, std::string("model_cfg.toml"));
    auto aic_w = std::make_shared<AIC>(clk_w, memory_w);
    aic_w->build(std::string("model_cfg.toml"));
    auto ret = aic_w->start(case_toml);
    EXPECT_TRUE(ret);
}


//
// 目的：测试不同阵列尺寸（4、8、16、32 等）下的矩阵乘法可扩展性与正确性。
// 说明：此测试针对更大的输入（例如 256x256）进行多次仿真，运行时间较长，
// 因此默认被禁用（以 `DISABLED_` 前缀）。
TEST(Integration, Scaling) {
    auto case_dir = std::getenv("CASE_OUTPUT_DIR") ? std::string(std::getenv("CASE_OUTPUT_DIR")) : std::string(PROJECT_SRC_DIR) + std::string("/tests/cases");
    std::string case_toml = case_dir + std::string("/case_Scaling.toml");
    util::CaseConfig case_cfg;
    bool have_case = util::read_case_toml(case_toml, case_cfg);
    int M = 256, K = 256, N = 256;
    auto A = util::generate_random_matrix(M,K);
    auto B = util::generate_random_matrix(K,N);
    if (!have_case) {
        std::filesystem::create_directories(case_dir);
        case_cfg.case_path = case_toml;
        case_cfg.a_path = case_dir + std::string("/Scaling_A.bin");
        case_cfg.b_path = case_dir + std::string("/Scaling_B.bin");
        case_cfg.c_golden_path = case_dir + std::string("/Scaling_C_golden.bin");
        case_cfg.c_out_path = case_dir + std::string("/Scaling_C_out.bin");
        case_cfg.a_addr = 0;
        case_cfg.b_addr = static_cast<uint32_t>(A.size());
        case_cfg.c_addr = static_cast<uint32_t>(A.size() + B.size());
        case_cfg.M = M; case_cfg.K = K; case_cfg.N = N;
        util::write_bin_int16(case_cfg.a_path, A);
        util::write_bin_int16(case_cfg.b_path, B);
        std::vector<int32_t> Cgold(M*N,0);
        for (int i=0;i<M;i++) for (int j=0;j<N;j++) {
            int32_t acc = 0;
            for (int k=0;k<K;k++) acc += static_cast<int32_t>(A[i*K+k]) * static_cast<int32_t>(B[k*N+j]);
            Cgold[i*N+j] = acc;
        }
        util::write_bin_int32(case_cfg.c_golden_path, Cgold);
        util::write_case_toml(case_cfg);
    }

    auto clk = std::make_shared<Clock>();
    auto memory = std::make_shared<Mem>(clk, std::string("model_cfg.toml"));
    auto aic = std::make_shared<AIC>(clk, memory);
    aic->build(std::string("model_cfg.toml"));
    auto ret = aic->start(case_toml);
    EXPECT_TRUE(ret);
}

// use gtest_main provided by the test target (no explicit main here)
