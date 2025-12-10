cmake_minimum_required(VERSION 3.10)
project(x_sim CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -g -O0)

# Split sources into a library (core) and the main executable so tests can link the core.
set(SRCS_LIB
    systolic.cpp
    pe.cpp
    mem_if.cpp
    clock.cpp
    cube.cpp
    aic.cpp
    config/mini_toml.cpp
    config/config_mgr.cpp
)

add_library(x_sim_lib ${SRCS_LIB})
target_include_directories(x_sim_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()

# The core library `x_sim_lib` exposes the simulator functionality.
# There is no longer a bundled `x_sim` executable in-tree; tests
# link against `x_sim_lib` and external tools can link to this library.

# Add tests directory (GoogleTest will be fetched by tests/CMakeLists)
add_subdirectory(tests)
