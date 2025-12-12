#include <gtest/gtest.h>
#include "util/utils.h"
#include "util/case_io.h"
#include <filesystem>

// 简单单元测试以确保 verify_result 的正确性和边界行为
TEST(VerifyUtil, Small2x2Identity) {
    std::vector<int16_t> A = {1,2,3,4}; // 2x2
    std::vector<int16_t> B = {1,0,0,1}; // 2x2 identity
    std::vector<int32_t> C(4);
    // manual multiply
    C[0] = 1*1 + 2*0;
    C[1] = 1*0 + 2*1;
    C[2] = 3*1 + 4*0;
    C[3] = 3*0 + 4*1;
    EXPECT_TRUE(util::verify_result(A, 2, 2, B, 2, 2, C));
}

TEST(VerifyUtil, MismatchedDimensions) {
    std::vector<int16_t> A = {1,2,3}; // 1x3
    std::vector<int16_t> B = {1,2,3,4}; // 2x2
    std::vector<int32_t> C(2);
    EXPECT_FALSE(util::verify_result(A, 1, 3, B, 2, 2, C));
}

TEST(VerifyUtil, ZeroSized) {
    std::vector<int16_t> A;
    std::vector<int16_t> B;
    std::vector<int32_t> C;
    EXPECT_FALSE(util::verify_result(A, 0, 0, B, 0, 0, C));
}

TEST(BinIO, RoundTripInt16) {
    auto tmp = std::filesystem::temp_directory_path() / "x_sim_test_binio_int16.bin";
    std::vector<int16_t> in = {1,2,3,4,5};
    EXPECT_TRUE(util::write_bin<int16_t>(tmp.string(), in));
    std::vector<int16_t> out;
    EXPECT_TRUE(util::read_bin<int16_t>(tmp.string(), out));
    EXPECT_EQ(in.size(), out.size());
    for (size_t i=0;i<in.size();++i) EXPECT_EQ(in[i], out[i]);
}

TEST(VerifyUtil, ComputeDiffs) {
    std::vector<int32_t> a = {1,2,3,4};
    std::vector<int32_t> b = {1,9,3,8};
    auto diffs = util::compute_diffs(a,b);
    EXPECT_EQ(diffs.size(), 2u);
    EXPECT_EQ(diffs[0], 1u);
    EXPECT_EQ(diffs[1], 3u);
}

TEST(VerifyUtil, ComputeReferenceSmall) {
    std::vector<int16_t> A = {1,2,3,4}; // 2x2
    std::vector<int16_t> B = {5,6,7,8}; // 2x2
    // reference: A * B = [[1*5+2*7, 1*6+2*8], [3*5+4*7, 3*6+4*8]]
    std::vector<int32_t> expected = {19,22,43,50};
    auto ref = util::compute_reference(A, 2, 2, B, 2);
    ASSERT_EQ(ref.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) EXPECT_EQ(ref[i], expected[i]);
}
