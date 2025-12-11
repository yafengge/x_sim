# x_sim — 构建与测试说明

本文件简要说明如何在本仓库中构建模拟器并运行测试（中文）。

构建（默认允许通过 FetchContent 获取 GoogleTest）

```bash
cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

测试

默认情况下，测试目标会使用系统安装的 GoogleTest（若可用）；若系统没有安装，CMake 会通过 `FetchContent` 自动下载并构建 GoogleTest（默认行为）。

运行测试：

```bash
# 运行所有已注册的测试（默认跳过 DISABLED_ 前缀的慢测试）
ctest --test-dir build -V

# 直接运行测试可执行并启用所有被禁用的测试（快速本地检查）
./build/tests/x_sim_tests --gtest_also_run_disabled_tests
```

CMake 选项

- `-DUSE_FETCH_GTEST=OFF`：要求系统安装 GoogleTest，否则配置会失败。默认：`ON`（允许 FetchContent 下载）。
- `-DENABLE_SLOW_TESTS=ON`：启用慢/扩展测试。默认：`OFF`。开启后，CTest 会在注册测试时加入 `--gtest_also_run_disabled_tests` 参数，从而运行以 `DISABLED_` 开头的测试用例。

示例（要求系统 GTest，并同时启用慢测试）：

```bash
cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug -DUSE_FETCH_GTEST=OFF -DENABLE_SLOW_TESTS=ON
cmake --build build -j4 --target x_sim_tests
ctest --test-dir build -V
```

如何查看或运行单个测试

使用 `--gtest_filter`：

```bash
./build/tests/x_sim_tests --gtest_filter=Integration.SmallMatrix
```

建议

- CI 中通常建议安装系统的 GoogleTest（或缓存第三方依赖）、并保留 `ENABLE_SLOW_TESTS=OFF` 以避免长时间运行的慢测。
- 如果你希望在本地调试慢测试，可临时使用 `--gtest_also_run_disabled_tests` 或将 `ENABLE_SLOW_TESTS` 打开重新构建。
