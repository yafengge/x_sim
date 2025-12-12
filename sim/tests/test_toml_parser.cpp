#include <gtest/gtest.h>
#include "config/config.h"
#include <filesystem>
#include <fstream>

TEST(TomlParser, ArrayFlatten) {
    std::string tmp = std::filesystem::temp_directory_path().string() + "/tp_array.toml";
    std::ofstream ofs(tmp);
    ofs << "arr = [\"one\", \"two\", 3, true]\n";
    ofs.close();

    auto m = config::TomlParser::parse_file(tmp);
    EXPECT_FALSE(m.empty());
    EXPECT_EQ(m.at("arr"), "one,two,3,true");
}

TEST(TomlParser, NestedTables) {
    std::string tmp = std::filesystem::temp_directory_path().string() + "/tp_nested.toml";
    std::ofstream ofs(tmp);
    ofs << "[a.b.c]\nval = \"x\"\n";
    ofs.close();

    auto m = config::TomlParser::parse_file(tmp);
    EXPECT_FALSE(m.empty());
    EXPECT_EQ(m.at("a.b.c.val"), "x");
}

TEST(TomlParser, ParseErrorReturnsEmpty) {
    std::string tmp = std::filesystem::temp_directory_path().string() + "/tp_bad.toml";
    std::ofstream ofs(tmp);
    ofs << "this is not toml = [";
    ofs.close();

    auto m = config::TomlParser::parse_file(tmp);
    EXPECT_TRUE(m.empty());
}
