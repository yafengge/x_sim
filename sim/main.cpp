#include "systolic.h"
#include "cube.h"
#include "sim_top.h"
#include "config/config_mgr.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <random>
#include <chrono>
#include <string>
#include <iomanip>

static bool g_verbose = false;
static int g_trace_cycles = 0;
static std::string g_config_path = "config/model.toml";

// `main` is intentionally minimal: this binary is the simulator entrypoint.
// All automated tests live in `tests/` and are executed via the test runner.
int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--verbose" || arg == "-v") g_verbose = true;
        else if (arg.rfind("--trace=", 0) == 0) {
            g_trace_cycles = std::stoi(arg.substr(8));
            if (g_trace_cycles > 0) g_verbose = true;
        } else if (arg.rfind("--config=", 0) == 0) {
            g_config_path = arg.substr(9);
        }
    }

    std::cout << "x_sim simulator (executable entrypoint)." << std::endl;
    std::cout << "Run unit/integration tests with ctest or the test runner." << std::endl;
    (void)g_verbose; (void)g_trace_cycles; (void)g_config_path;
    return 0;
}