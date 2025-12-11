#include <gtest/gtest.h>
#include "util/verify.h"

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
    EXPECT_TRUE(xsim::util::verify_result(A, 2, 2, B, 2, 2, C));
}

TEST(VerifyUtil, MismatchedDimensions) {
    std::vector<int16_t> A = {1,2,3}; // 1x3
    std::vector<int16_t> B = {1,2,3,4}; // 2x2
    std::vector<int32_t> C(2);
    EXPECT_FALSE(xsim::util::verify_result(A, 1, 3, B, 2, 2, C));
}

TEST(VerifyUtil, ZeroSized) {
    std::vector<int16_t> A;
    std::vector<int16_t> B;
    std::vector<int32_t> C;
    EXPECT_FALSE(xsim::util::verify_result(A, 0, 0, B, 0, 0, C));
}
