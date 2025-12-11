// aic.cpp — AIC 顶层实现（中文注释）
// 本文件实现 AIC 类的组装逻辑：创建时钟、内存并构建 Cube 实例。
// 注意：此处仅为模块组装，不包含仿真的内部细节，这些在子模块中实现。

#include "aic.h"

#include <iostream>
#include "util/case_io.h"
#include <filesystem>

static void print_diffs(const std::vector<int32_t>& a, const std::vector<int32_t>& b) {
  size_t n = std::min(a.size(), b.size());
  size_t diffs = 0;
  for (size_t i = 0; i < n; ++i) {
    if (a[i] != b[i]) {
      if (diffs < 20) {
        std::cerr << "diff at " << i << ": expected=" << b[i] << " got=" << a[i] << "\n";
      }
      ++diffs;
    }
  }
  if (a.size() != b.size()) {
    std::cerr << "size mismatch: got=" << a.size() << " golden=" << b.size() << "\n";
  }
  std::cerr << "total differences: " << diffs << "\n";
}

AIC::AIC(const p_clock_t& clk,
         const p_mem_t& mem)
  : clk_(clk), mem_(mem) {
  // Delayed: cube_ will be constructed in build(config_path)
}

void AIC::build(const std::string& config_path, bool force) {
  if (!clk_ || !mem_) {
    std::cerr << "AIC::build: clock or memory not provided" << std::endl;
    return;
  }

  // If cube not yet constructed, or config changed, or force requested,
  // create/recreate the Cube instance.
  if (!cube_ || force || config_path != config_path_) {
    config_path_ = config_path;
    cube_.reset();
    cube_ = p_cube_t(new Cube(config_path_, clk_, mem_));
  }
}

bool AIC::start(const std::vector<DataType>& A, int A_rows, int A_cols,
                const std::vector<DataType>& B, int B_rows, int B_cols,
                std::vector<AccType>& C) {
  if (!cube_) {
    std::cerr << "AIC::start: cube not constructed; call build(config_path) first" << std::endl;
    return false;
  }
  return cube_->run(A, A_rows, A_cols, B, B_rows, B_cols, C);
}

bool AIC::start(const std::string &case_toml_path) {
  util::CaseConfig cfg;
  if (!util::read_case_toml(case_toml_path, cfg)) {
    std::cerr << "AIC::start: failed to read case toml: " << case_toml_path << std::endl;
    return false;
  }

  // read input binaries (try raw path first; if missing and path is relative,
  // retry by prefixing PROJECT_SRC_DIR so TOMLs with "tests/cases/..." work
  // regardless of the current working directory).
  std::vector<DataType> A;
  std::vector<DataType> B;
  std::vector<AccType> C;

  auto try_read_int16 = [&](const std::string &path, std::vector<DataType> &out) -> bool {
    if (util::read_bin_int16(path, out)) return true;
    std::filesystem::path p(path);
    // Try resolving relative paths by walking up from current path, and also
    // try PROJECT_SRC_DIR as a fallback.
    if (p.is_relative()) {
      std::filesystem::path cwd = std::filesystem::current_path();
      for (int depth = 0; depth < 5; ++depth) {
        std::string candidate = (cwd / path).string();
        if (util::read_bin_int16(candidate, out)) return true;
        if (cwd.has_parent_path()) cwd = cwd.parent_path();
        else break;
      }
      std::string alt = std::string(PROJECT_SRC_DIR) + std::string("/") + path;
      if (util::read_bin_int16(alt, out)) return true;
    }
    return false;
  };

  if (!try_read_int16(cfg.a_path, A)) {
    std::cerr << "AIC::start: failed to read A from " << cfg.a_path << std::endl;
    return false;
  }
  if (!try_read_int16(cfg.b_path, B)) {
    std::cerr << "AIC::start: failed to read B from " << cfg.b_path << std::endl;
    return false;
  }

  int A_rows = cfg.M; int A_cols = cfg.K;
  int B_rows = cfg.K; int B_cols = cfg.N;

  // Preload A/B into the memory model at the addresses declared in the case
  // configuration so each tile does not need to re-request the data.
  if (mem_) {
    mem_->load_data(A, cfg.a_addr);
    mem_->load_data(B, cfg.b_addr);
  } else {
    std::cerr << "AIC::start: mem_ is null; cannot preload A/B" << std::endl;
  }

  if (!cube_) {
    std::cerr << "AIC::start: cube not constructed; call build(config_path) first" << std::endl;
    return false;
  }

  bool ok = cube_->run(A, A_rows, A_cols, B, B_rows, B_cols, C);
  if (!ok) return false;

  // write C out (resolve relative paths similar to reads)
  auto resolve_path = [&](const std::string &path)->std::string {
    std::filesystem::path p(path);
    if (p.is_absolute()) return path;
    return std::string(PROJECT_SRC_DIR) + std::string("/") + path;
  };

  std::string c_out_resolved = resolve_path(cfg.c_out_path);
  if (!util::write_bin_int32(c_out_resolved, C)) {
    std::cerr << "AIC::start: failed to write C_out to " << c_out_resolved << std::endl;
    // continue to allow comparison if golden exists
  }

  // compare with golden if present
  std::vector<int32_t> Cgold;
  if (!cfg.c_golden_path.empty()) {
    std::string c_golden_resolved = resolve_path(cfg.c_golden_path);
    if (util::read_bin_int32(c_golden_resolved, Cgold)) {
      if (Cgold.size() != C.size()) {
        std::cerr << "AIC::start: golden size mismatch: " << Cgold.size() << " vs " << C.size() << std::endl;
        return false;
      }
      bool match = true;
      for (size_t i = 0; i < C.size(); ++i) {
        if (C[i] != Cgold[i]) { match = false; break; }
      }
      if (!match) {
        std::cerr << "AIC::start: result does not match golden for case " << case_toml_path << std::endl;
        print_diffs(C, Cgold);
        return false;
      }
    } else {
      std::cerr << "AIC::start: could not read golden file " << cfg.c_golden_path << std::endl;
    }
  }

  return true;
}
