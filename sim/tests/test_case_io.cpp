#include <gtest/gtest.h>
#include "util/case_io.h"
#include "config/config.h"
#include <filesystem>

using namespace util;

TEST(CaseIO, ParseRelativePaths) {
    // Create a temporary TOML file
    std::string tmpdir = std::filesystem::temp_directory_path().string();
    std::string case_path = tmpdir + "/test_case_io.toml";
    std::ofstream ofs(case_path);
    ofs << "[input.A]\n";
    ofs << "path = \"a.bin\"\n";
    ofs << "addr = 0\n";
    ofs << "[input.B]\n";
    ofs << "path = \"b.bin\"\n";
    ofs << "addr = 1\n";
    ofs << "[output.C]\n";
    ofs << "golden = \"c_gold.bin\"\n";
    ofs << "out = \"c_out.bin\"\n";
    ofs << "[meta]\n";
    ofs << "M = 2\nK = 2\nN = 2\n";
    ofs.close();

    CaseConfig cfg;
    bool ok = read_case_toml(case_path, cfg);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(cfg.a_path.empty());
    EXPECT_FALSE(cfg.b_path.empty());
    // paths should be absolute after parsing
    EXPECT_TRUE(std::filesystem::path(cfg.a_path).is_absolute());
    EXPECT_EQ(cfg.M, 2);
}

TEST(TomlParser, BasicParse) {
    std::string tmpdir = std::filesystem::temp_directory_path().string();
    std::string case_path = tmpdir + "/test_toml_parser.toml";
    std::ofstream ofs(case_path);
    ofs << "[cube]\narray_rows = 4\narray_cols = 4\n";
    ofs.close();
    auto m = config::TomlParser::parse_file(case_path);
    EXPECT_FALSE(m.empty());
    EXPECT_EQ(m.at("cube.array_rows"), "4");
}
