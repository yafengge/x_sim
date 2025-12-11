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
