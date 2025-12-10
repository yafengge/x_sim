cmake_minimum_required(VERSION 3.10)
project(x_sim CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default compile flags for debug-like build (match previous Makefile)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -g -O0)

# Sources (keep in same order as previous Makefile)
set(SRCS
  systolic.cpp
  main.cpp
  pe.cpp
  mem_if.cpp
  clock.cpp
  cube.cpp
  sim_top.cpp
  config/mini_toml.cpp
  config/config_mgr.cpp
)

add_executable(x_sim ${SRCS})

target_include_directories(x_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Provide a 'run-quick' custom target for convenience
add_custom_target(run-quick
  COMMAND $<TARGET_FILE:x_sim> --quick
  DEPENDS x_sim
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
