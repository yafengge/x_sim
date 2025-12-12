#include "util/log.h"
#include "util/utils.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>

namespace util {

static std::shared_ptr<spdlog::logger> global_logger = nullptr;

bool log_init(const std::string &log_dir, const std::string &level, bool async, size_t rotate_size, int rotate_count) {
    try {
        std::filesystem::create_directories(log_dir);
    } catch(...) {}

    std::string logfile = (std::filesystem::path(log_dir) / "x_sim.log").string();

    try {
        // parse level
        spdlog::level::level_enum lvl = spdlog::level::info;
        std::string up = util::to_lower(level);
        if (up == "debug") lvl = spdlog::level::debug;
        else if (up == "info") lvl = spdlog::level::info;
        else if (up == "warn" || up == "warning") lvl = spdlog::level::warn;
        else if (up == "error") lvl = spdlog::level::err;

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logfile, rotate_size, rotate_count));

        // Create synchronous logger with console + rotating file sinks.
        global_logger = std::make_shared<spdlog::logger>("x_sim", sinks.begin(), sinks.end());
        spdlog::set_default_logger(global_logger);
        global_logger->set_level(lvl);
        global_logger->flush_on(spdlog::level::err);
        return true;
    } catch(...) {
        return false;
    }
}

void log_shutdown() {
    try {
        if (global_logger) {
            spdlog::drop_all();
            global_logger.reset();
        }
        spdlog::shutdown();
    } catch(...) {}
}

std::shared_ptr<spdlog::logger> log_get() { return global_logger; }

} // namespace util
