cmake_minimum_required(VERSION 3.10)

include(CheckIncludeFile)

# Option: allow toggling whether to fetch GoogleTest when a system package is
# not available. Set -DUSE_FETCH_GTEST=OFF to require a system-installed GTest.
option(USE_FETCH_GTEST "Fetch GoogleTest via FetchContent if system GTest not present" ON)
option(ENABLE_SLOW_TESTS "Enable slow/extended tests (runs disabled tests)" OFF)

# Try to find a system-installed GTest first.
find_package(GTest QUIET)

if (GTest_FOUND)
  message(STATUS "Using system GoogleTest")
elseif (USE_FETCH_GTEST)
  include(FetchContent)
  message(STATUS "GoogleTest not found; fetching GoogleTest via FetchContent")
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG main
  )
  # Prevent overriding the parent project's compiler/linker settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
else()
  message(FATAL_ERROR "GoogleTest not found and USE_FETCH_GTEST=OFF. Install system GTest or set -DUSE_FETCH_GTEST=ON to fetch it.")
endif()

add_executable(x_sim_tests test_integration.cpp)
target_compile_definitions(x_sim_tests PRIVATE USE_GTEST=1)
target_link_libraries(x_sim_tests PRIVATE x_sim_lib GTest::gtest_main)
include(GoogleTest)

if (ENABLE_SLOW_TESTS)
  message(STATUS "ENABLE_SLOW_TESTS=ON: tests will run including disabled (slow) tests via a single CTest entry")
  # Register a single CTest that runs the entire test binary and enables disabled tests.
  add_test(NAME x_sim_all COMMAND $<TARGET_FILE:x_sim_tests> --gtest_also_run_disabled_tests)
else()
  gtest_discover_tests(x_sim_tests)
endif()
