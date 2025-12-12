#include <gtest/gtest.h>
#include "util/log.h"
#include <filesystem>

TEST(Logging, InitAndWrite) {
    auto tmp = std::filesystem::temp_directory_path() / "x_sim_test_logs";
    std::string dir = tmp.string();
    // initialize
    ASSERT_TRUE(util::log_init(dir, "debug", true, 1024*1024, 2));
    LOG_INFO("test message {}", 1);
    LOG_DEBUG("debug message {}", 2);
    util::log_shutdown();
    // check file exists
    auto f = tmp / "x_sim.log";
    ASSERT_TRUE(std::filesystem::exists(f));
}
