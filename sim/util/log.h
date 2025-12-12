#pragma once
#include <memory>
#include <string>
#include <spdlog/logger.h>

namespace util {

// Initialize logging subsystem. `log_dir` is a directory path where
// rotating log files will be written (file name: x_sim.log).
// `level` is a spdlog level string like "info", "debug", "warn", "error".
// q_size: async queue size (items). thread_count: number of background threads.
bool log_init(const std::string &log_dir, const std::string &level = "info", bool async = true,
              size_t rotate_size = 10 * 1024 * 1024, int rotate_count = 5,
              size_t q_size = 8192, size_t thread_count = 1);

// Shutdown logging gracefully.
void log_shutdown();

// Get the global logger (may be null if not initialized).
std::shared_ptr<spdlog::logger> log_get();

} // namespace util

// convenience macros
#define LOG_DEBUG(...) if(auto _l = util::log_get()) _l->debug(__VA_ARGS__)
#define LOG_INFO(...)  if(auto _l = util::log_get()) _l->info(__VA_ARGS__)
#define LOG_WARN(...)  if(auto _l = util::log_get()) _l->warn(__VA_ARGS__)
#define LOG_ERROR(...) if(auto _l = util::log_get()) _l->error(__VA_ARGS__)
