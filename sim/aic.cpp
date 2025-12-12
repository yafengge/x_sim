// aic.cpp — AIC 顶层实现（中文注释）
// 本文件实现 AIC 类的组装逻辑：创建时钟、内存并构建 Cube 实例。
// 注意：此处仅为模块组装，不包含仿真的内部细节，这些在子模块中实现。

#include "aic.h"

#include <iostream>
#include "util/case_io.h"
#include <fstream>
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

// Helper implementations

// Read A/B binaries according to an already-populated CaseConfig
bool AIC::read_bins_from_cfg(const util::CaseConfig &cfg, std::vector<DataType> &A, std::vector<DataType> &B) {
  auto try_read_int16 = [&](const std::string &path, std::vector<DataType> &out) -> bool {
    if (util::read_bin_int16(path, out)) return true;
    std::filesystem::path p(path);
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
  return true;
}

std::string AIC::resolve_path(const std::string &path) const {
  std::filesystem::path p(path);
  if (p.is_absolute()) return path;
  return std::string(PROJECT_SRC_DIR) + std::string("/") + path;
}

void AIC::preload_into_mem(const util::CaseConfig &cfg, const std::vector<DataType> &A, const std::vector<DataType> &B) {
  if (mem_) {
    mem_->load_data(A, cfg.a_addr);
    mem_->load_data(B, cfg.b_addr);
  } else {
    std::cerr << "AIC::start: mem_ is null; cannot preload A/B" << std::endl;
  }
}

bool AIC::write_and_compare(const util::CaseConfig &cfg, const std::vector<AccType> &C,
                            const std::vector<DataType> &A, const std::vector<DataType> &B) {
  std::string c_out_resolved = resolve_path(cfg.c_out_path);
  if (!util::write_bin_int32(c_out_resolved, C)) {
    std::cerr << "AIC::start: failed to write C_out to " << c_out_resolved << std::endl;
  }

  if (!cfg.c_golden_path.empty()) {
    std::string c_golden_resolved = resolve_path(cfg.c_golden_path);
    std::vector<int32_t> Cgold;
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
        std::cerr << "AIC::start: result does not match golden for case " << cfg.case_path << std::endl;
        std::vector<size_t> diffs_idx;
        for (size_t ii = 0; ii < C.size(); ++ii) {
          if (C[ii] != Cgold[ii]) diffs_idx.push_back(ii);
        }
        size_t show = std::min<size_t>(diffs_idx.size(), 10);
        for (size_t d = 0; d < show; ++d) {
          size_t idx = diffs_idx[d];
          size_t row = cfg.M > 0 ? idx / cfg.N : 0;
          size_t col = cfg.N > 0 ? idx % cfg.N : 0;
          std::cerr << "diff[" << d << "] idx=" << idx << " (r=" << row << ",c=" << col << ") expected=" << Cgold[idx] << " got=" << C[idx] << "\n";
          std::cerr << "  A[row] = [";
          for (int k = 0; k < cfg.K; ++k) {
            if (k) std::cerr << ",";
            std::cerr << A[row * cfg.K + k];
          }
          std::cerr << "]\n";
          std::cerr << "  B[col] = [";
          for (int k = 0; k < cfg.K; ++k) {
            if (k) std::cerr << ",";
            std::cerr << B[k * cfg.N + col];
          }
          std::cerr << "]\n";
        }

        std::string diff_path;
        if (!cfg.case_path.empty()) diff_path = cfg.case_path + std::string(".diff");
        else diff_path = cfg.c_out_path + std::string(".diff");
        std::ofstream df(diff_path);
        if (df) {
          df << "case=" << cfg.case_path << "\n";
          df << "total_diffs=" << diffs_idx.size() << "\n";
          for (size_t idx : diffs_idx) {
            size_t row = cfg.M > 0 ? idx / cfg.N : 0;
            size_t col = cfg.N > 0 ? idx % cfg.N : 0;
            df << idx << "," << row << "," << col << ",got=" << C[idx] << ",exp=" << Cgold[idx] << "\n";
            df << "A_row:";
            for (int k = 0; k < cfg.K; ++k) { if (k) df << ","; df << A[row * cfg.K + k]; }
            df << "\nB_col:";
            for (int k = 0; k < cfg.K; ++k) { if (k) df << ","; df << B[k * cfg.N + col]; }
            df << "\n---\n";
          }
          df.close();
          std::cerr << "AIC::start: wrote diff file: " << diff_path << std::endl;
        }

        print_diffs(C, Cgold);
        return false;
      }
    } else {
      std::cerr << "AIC::start: could not read golden file " << cfg.c_golden_path << std::endl;
    }
  }
  return true;
}

AIC::AIC(const p_clock_t& clk,
         const p_mem_t& mem)
  : clk_(clk), mem_(mem) {
  // Delayed: cube_ will be constructed in build(config_path)
}

void AIC::build(const std::string& case_toml_path, bool force) {
  if (!clk_ || !mem_) {
    std::cerr << "AIC::build: clock or memory not provided" << std::endl;
    return;
  }

  // Read and store the case config. This also contains a reference to the
  // platform model config (model_cfg.toml) which we will use to construct
  // the Cube.
  util::CaseConfig cfg;
  if (!util::read_case_toml(case_toml_path, cfg)) {
    std::cerr << "AIC::build: failed to read case toml: " << case_toml_path << std::endl;
    return;
  }
  case_cfg_ = cfg;

  // Determine model config path referenced by the case. If it's empty,
  // fall back to default "model_cfg.toml" in project root.
  std::string model_cfg = cfg.model_cfg_path.empty() ? std::string("model_cfg.toml") : cfg.model_cfg_path;

  // If cube not yet constructed, or model config changed, or force requested,
  // create/recreate the Cube instance.
  if (!cube_ || force || model_cfg != config_path_) {
    config_path_ = model_cfg;
    cube_.reset();
    cube_ = p_cube_t(new Cube(config_path_, clk_, mem_));
  }
}

// (removed) legacy overload AIC::start(A,B) — use AIC::start() with case TOML

bool AIC::start() {
  if (case_cfg_.case_path.empty()) {
    std::cerr << "AIC::start: no case configured; call build(case_toml) first" << std::endl;
    return false;
  }
  std::vector<DataType> A;
  std::vector<DataType> B;
  std::vector<AccType> C;

  if (!read_bins_from_cfg(case_cfg_, A, B)) return false;
  int A_rows = case_cfg_.M; int A_cols = case_cfg_.K;
  int B_rows = case_cfg_.K; int B_cols = case_cfg_.N;

  preload_into_mem(case_cfg_, A, B);

  if (!cube_) {
    std::cerr << "AIC::start: cube not constructed; call build(case_toml) first" << std::endl;
    return false;
  }

  // Run the cube using memory-resident inputs. The Cube/SystolicArray will
  // read A/B from memory at the configured addresses and commit results into
  // the accumulator region starting at cfg.c_addr. After the run, dump the
  // accumulator region back into `C` for writing/comparison.
  bool ok = cube_->run_from_memory(case_cfg_.M, case_cfg_.N, case_cfg_.K,
                                   case_cfg_.a_addr, case_cfg_.b_addr, case_cfg_.c_addr);
  if (!ok) return false;

  // Read results back from memory
  size_t c_len = static_cast<size_t>(case_cfg_.M) * static_cast<size_t>(case_cfg_.N);
  std::vector<AccType> Cacc;
  if (!mem_->dump_acc(case_cfg_.c_addr, c_len, Cacc)) {
    std::cerr << "AIC::start: failed to dump accumulator memory at " << case_cfg_.c_addr << std::endl;
    return false;
  }

  if (!write_and_compare(case_cfg_, Cacc, A, B)) return false;

  return true;
}
