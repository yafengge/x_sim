cmake_minimum_required(VERSION 3.10)
project(x_sim CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -g -O0)

# Fetch toml++ (tomlplusplus) for optional TOML support. This is optional
# at runtime; if FetchContent is able to obtain toml++, we expose the
# `tomlplusplus::tomlplusplus` target and define HAVE_TOMLPP for consumers.
include(FetchContent)
FetchContent_Declare(
  tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
)
FetchContent_MakeAvailable(tomlplusplus)

# Require tomlplusplus for the build; define HAVE_TOMLPP for consumers.
if(NOT TARGET tomlplusplus::tomlplusplus)
  message(FATAL_ERROR "Failed to fetch tomlplusplus; toml support is required")
endif()

# Split sources into a library (core) and the main executable so tests can link the core.
set(SRCS_LIB
    systolic.cpp
    pe.cpp
    mem_if.cpp
    clock.cpp
    cube.cpp
    util/verify.cpp
    util/utils.cpp
    util/case_io.cpp
    aic.cpp
    
    config/config.cpp
)

add_library(x_sim_lib ${SRCS_LIB})
target_include_directories(x_sim_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET tomlplusplus::tomlplusplus)
  target_link_libraries(x_sim_lib PUBLIC tomlplusplus::tomlplusplus)
  target_compile_definitions(x_sim_lib PUBLIC HAVE_TOMLPP)
else()
  message(FATAL_ERROR "tomlplusplus target missing after FetchContent; build cannot proceed")
endif()
# Note: `PROJECT_SRC_DIR` compile definition removed. Path resolution now
# uses runtime current working directory or explicit environment variables.

enable_testing()

# The core library `x_sim_lib` exposes the simulator functionality.
# There is no longer a bundled `x_sim` executable in-tree; tests
# link against `x_sim_lib` and external tools can link to this library.

# Add tests directory (GoogleTest will be fetched by tests/CMakeLists)
add_subdirectory(tests)
