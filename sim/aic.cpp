
#include "aic.h"
#include "util/log.h"
#include "util/case_io.h"
#include "util/utils.h"
#include <fstream>
#include <filesystem>

// preload_into_mem moved to util/case_io

void AIC::preload_into_mem(const util::CaseConfig &cfg, const std::vector<DataType> &A, const std::vector<DataType> &B) {
  if (mem_) {
    mem_->load_data(A, cfg.a_addr);
    mem_->load_data(B, cfg.b_addr);
  } else {
    LOG_ERROR("AIC::preload_into_mem: mem_ is null; cannot preload A/B");
  }
}

AIC::AIC(const p_clock_t& clk,
         const p_mem_t& mem)
        : clk_(clk), mem_(mem) {
}

void AIC::build(const std::string& case_toml_path, bool force) {
  if (!clk_ || !mem_) {
    LOG_ERROR("AIC::build: clock or memory not provided");
    return;
  }

  // Read and store the case config. This also contains a reference to the
  // platform model config (model_cfg.toml) which we will use to construct
  // the Cube.
  util::CaseConfig cfg;
  if (!util::read_case_toml(case_toml_path, cfg)) {
    LOG_ERROR("AIC::build: failed to read case toml: {}", case_toml_path);
    return;
  }
  case_cfg_ = cfg;

  // If cube not yet constructed, or model config changed, or force requested,
  // create/recreate the Cube instance. Use case TOML's referenced model_cfg
  // or fall back to the default "model_cfg.toml".
  if (!cube_ || force) {
    cube_.reset();
    cube_ = p_cube_t(new Cube(clk_, mem_));
  }
}

bool AIC::start() {
  if (case_cfg_.case_path.empty()) {
    LOG_ERROR("AIC::start: no case configured; call build(case_toml) first");
    return false;
  }
  std::vector<DataType> A;
  std::vector<DataType> B;
  std::vector<AccType> C;

  if (!util::read_bins_from_cfg(case_cfg_, A, B)) return false;
  (void)case_cfg_; // avoid unused warning if fields not referenced later

  preload_into_mem(case_cfg_, A, B);

  if (!cube_) {
    LOG_ERROR("AIC::start: cube not constructed; call build(case_toml) first");
    return false;
  }

  bool ok = cube_->run(case_cfg_.M, case_cfg_.N, case_cfg_.K,
                       case_cfg_.a_addr, case_cfg_.b_addr, case_cfg_.c_addr);
  if (!ok) return false;

  // Read results back from memory
  size_t c_len = static_cast<size_t>(case_cfg_.M) * static_cast<size_t>(case_cfg_.N);
  std::vector<AccType> Cacc;
  if (!mem_->dump_acc(case_cfg_.c_addr, c_len, Cacc)) {
    LOG_ERROR("AIC::start: failed to dump accumulator memory at {}", case_cfg_.c_addr);
    return false;
  }

  if (!util::write_and_compare(case_cfg_, Cacc, A, B)) return false;

  return true;
}
