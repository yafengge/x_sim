Case fixtures for integration tests

Purpose
- Store binary input/output files and a small TOML describing where to load them from.
- Tests and `AIC::start` read these TOML files to locate and preload A/B into the memory model.

TOML schema (fields used by code)

[input.A]
path = "<path-to-A.bin>"    # path to A.bin (absolute recommended)
addr = <uint32>              # address (element index) where A should be loaded
type = "int16"             # data type of A elements

[input.B]
path = "<path-to-B.bin>"
addr = <uint32>
type = "int16"

[output.C]
golden = "<path-to-C_golden.bin>"  # reference output
out = "<path-to-C_out.bin>"          # where AIC will write C_out
addr = <uint32>
type = "int32"

[meta]
M = <rows-of-A>
K = <common-dim>
N = <cols-of-B>
endian = "little"  # byte order

Recommendations
- Prefer absolute paths in the TOML so consumers do not depend on current working directory.
- In CI, set `CASE_OUTPUT_DIR` to a writable path under the build directory to avoid polluting the source tree.

How to run
- You can run the integration cases in two ways:
- Using `ctest` (recommended when running the full suite or single tests):
	- From the repository root:

```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target x_sim_tests
ctest --test-dir . --output-on-failure -R Integration.SmallMatrix
```

- Or run the test binary directly and filter by GoogleTest name:

```bash
./build/x_sim_tests --gtest_filter=Integration.SmallMatrix
```

- Optional: set `CASE_OUTPUT_DIR` to redirect generated case binaries and output files to a directory under your build tree:

```bash
export CASE_OUTPUT_DIR="$PWD/build/tests/cases"
mkdir -p "$CASE_OUTPUT_DIR"
```

- Per-case, human-readable hints were moved into the test source; see `tests/test_integration.cpp` for short notes on how to run each case and which tests are disabled by default.

CI example (bash)

```bash
export CASE_OUTPUT_DIR="$BUILD_DIR/tests/cases"
mkdir -p "$CASE_OUTPUT_DIR"
cmake -B build -S .
cmake --build build --target x_sim_tests
build/tests/x_sim_tests --gtest_filter=Integration.*
```

Notes
- `util::write_case_toml` now writes absolute paths and updates the `CaseConfig` accordingly.
- `AIC::start` will attempt to resolve relative paths against the project source dir if necessary, but absolute paths are more robust.
