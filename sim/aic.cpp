
#include "aic.h"
#include <iostream>
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
    std::cerr << "AIC::preload_into_mem: mem_ is null; cannot preload A/B" << std::endl;
  }
}

AIC::AIC(const p_clock_t& clk,
         const p_mem_t& mem)
  : clk_(clk), mem_(mem) {
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

bool AIC::start() {
  if (case_cfg_.case_path.empty()) {
    std::cerr << "AIC::start: no case configured; call build(case_toml) first" << std::endl;
    return false;
  }
  std::vector<DataType> A;
  std::vector<DataType> B;
  std::vector<AccType> C;

  if (!util::read_bins_from_cfg(case_cfg_, A, B)) return false;
  (void)case_cfg_; // avoid unused warning if fields not referenced later

  preload_into_mem(case_cfg_, A, B);

  if (!cube_) {
    std::cerr << "AIC::start: cube not constructed; call build(case_toml) first" << std::endl;
    return false;
  }

  bool ok = cube_->run(case_cfg_.M, case_cfg_.N, case_cfg_.K,
                       case_cfg_.a_addr, case_cfg_.b_addr, case_cfg_.c_addr);
  if (!ok) return false;

  // Read results back from memory
  size_t c_len = static_cast<size_t>(case_cfg_.M) * static_cast<size_t>(case_cfg_.N);
  std::vector<AccType> Cacc;
  if (!mem_->dump_acc(case_cfg_.c_addr, c_len, Cacc)) {
    std::cerr << "AIC::start: failed to dump accumulator memory at " << case_cfg_.c_addr << std::endl;
    return false;
  }

  if (!util::write_and_compare(case_cfg_, Cacc, A, B)) return false;

  return true;
}
