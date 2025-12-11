#include <gtest/gtest.h>
#include "util/utils.h"
#include <filesystem>

TEST(UtilsIO, MatrixBinaryRoundtrip) {
    namespace fs = std::filesystem;
    auto tmp = fs::temp_directory_path() / "xsim_matrix_test.bin";
    std::vector<int16_t> A = xsim::util::generate_random_matrix(8,8);
    ASSERT_TRUE(xsim::util::write_matrix_bin(tmp.string(), A));
    std::vector<int16_t> B;
    ASSERT_TRUE(xsim::util::read_matrix_bin(tmp.string(), B));
    EXPECT_EQ(A.size(), B.size());
    EXPECT_EQ(A, B);
    fs::remove(tmp);
}

TEST(UtilsIO, CaseTomlRoundtrip) {
    namespace fs = std::filesystem;
    auto tmp = fs::temp_directory_path() / "xsim_case_test.toml";
    xsim::util::CaseMeta m;
    m.a_path = "a.bin"; m.a_rows = 4; m.a_cols = 4;
    m.b_path = "b.bin"; m.b_rows = 4; m.b_cols = 4;
    ASSERT_TRUE(xsim::util::write_case_toml(tmp.string(), m));
    xsim::util::CaseMeta m2;
    ASSERT_TRUE(xsim::util::read_case_toml(tmp.string(), m2));
    EXPECT_EQ(m.a_path, m2.a_path);
    EXPECT_EQ(m.a_rows, m2.a_rows);
    EXPECT_EQ(m.a_cols, m2.a_cols);
    EXPECT_EQ(m.b_path, m2.b_path);
    EXPECT_EQ(m.b_rows, m2.b_rows);
    EXPECT_EQ(m.b_cols, m2.b_cols);
    fs::remove(tmp);
}
