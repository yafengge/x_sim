// The test file supports two modes:
// - When compiled with USE_GTEST=1 it uses GoogleTest
// - Otherwise it runs a small fallback test runner so tests can still execute

#include <fstream>
#include <string>
#include <iostream>
#include "config/mini_toml.h"
#include "config/config_mgr.h"

#include <gtest/gtest.h>

TEST(MiniToml, ParseBasic) {
    std::string path = "test_config.toml";
    std::ofstream out(path);
    out << "[cube]\narray_rows = 4\narray_cols = 8\nverbose = true\n";
    out.close();

    auto m = parse_toml_file(path);
    ASSERT_TRUE(m.find("cube.array_rows") != m.end());
    EXPECT_EQ(m["cube.array_rows"], "4");
    EXPECT_EQ(m["cube.array_cols"], "8");
    EXPECT_EQ(m["cube.verbose"], "true");
}

TEST(ConfigMgr, Getters) {
    std::string path = "test_config2.toml";
    std::ofstream out(path);
    out << "[cube]\narray_rows = 2\narray_cols = 3\nverbose = false\n";
    out.close();

    int r = 0;
    EXPECT_TRUE(config_mgr::get_int_key(path, "cube.array_rows", r));
    EXPECT_EQ(r, 2);

    int c = 0;
    EXPECT_TRUE(config_mgr::get_int_key(path, "cube.array_cols", c));
    EXPECT_EQ(c, 3);

    bool v = true;
    EXPECT_TRUE(config_mgr::get_bool_key(path, "cube.verbose", v));
    EXPECT_FALSE(v);
}

// use gtest_main provided by the test target (no explicit main here)
