#include "util/case_io.h"
#include "util/utils.h"
#include "config/toml_adapter.h"
#include "mem_if.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <filesystem>

// 文件：util/case_io.cpp
// 说明：实现 case TOML 与二进制文件的创建/读取逻辑。
// 备注：尽量保持写入的路径为绝对路径，避免运行时依赖于工作目录。
namespace util {

bool write_config_file(const std::string& path, int array_rows, int array_cols) {
    std::ofstream fout(path);
    if (!fout) return false;
    fout << "[cube]\n";
    fout << "array_rows = " << array_rows << "\n";
    fout << "array_cols = " << array_cols << "\n";
    return fout.good();
}

// 将 CaseConfig 写为 TOML，写入时把二进制路径转换为绝对路径并写入文件。
bool write_case_toml(CaseConfig &cfg) {
    // 确保 TOML 所在目录存在
    try {
        std::filesystem::path case_p(cfg.case_path);
        if (case_p.has_parent_path()) std::filesystem::create_directories(case_p.parent_path());
    } catch (...) {
        // 忽略目录创建错误，后续写入会失败并返回 false
    }

    std::ofstream ofs(cfg.case_path);
    if (!ofs) return false;

    // 将二进制路径规范化为绝对路径，便于后续消费
    std::filesystem::path a_p = std::filesystem::absolute(cfg.a_path);
    std::filesystem::path b_p = std::filesystem::absolute(cfg.b_path);
    std::filesystem::path cgold_p = std::filesystem::absolute(cfg.c_golden_path);
    std::filesystem::path cout_p = std::filesystem::absolute(cfg.c_out_path);

    cfg.a_path = a_p.string();
    cfg.b_path = b_p.string();
    cfg.c_golden_path = cgold_p.string();
    cfg.c_out_path = cout_p.string();
    try {
        cfg.case_path = std::filesystem::absolute(cfg.case_path).string();
    } catch(...) {
        // 忽略
    }
    ofs << "# case toml generated\n";
    ofs << "[input.A]\n";
    ofs << "path = \"" << a_p.string() << "\"\n";
    ofs << "addr = " << cfg.a_addr << "\n";
    ofs << "type = \"" << cfg.a_type << "\"\n";
    ofs << "[input.B]\n";
    ofs << "path = \"" << b_p.string() << "\"\n";
    ofs << "addr = " << cfg.b_addr << "\n";
    ofs << "type = \"" << cfg.b_type << "\"\n";
    ofs << "[output.C]\n";
    ofs << "golden = \"" << cgold_p.string() << "\"\n";
    ofs << "out = \"" << cout_p.string() << "\"\n";
    ofs << "addr = " << cfg.c_addr << "\n";
    ofs << "type = \"" << cfg.c_type << "\"\n";
    ofs << "[meta]\n";
    ofs << "M = " << cfg.M << "\n";
    ofs << "K = " << cfg.K << "\n";
    ofs << "N = " << cfg.N << "\n";
    ofs << "endian = \"" << cfg.endian << "\"\n";
    // model_cfg 单独放在一个表中，便于调用方独立引用平台配置
    ofs << "[model_cfg]\n";
    if (!cfg.model_cfg_path.empty()) {
        ofs << "path = \"" << cfg.model_cfg_path << "\"\n";
    } else {
        ofs << "path = \"model_cfg.toml\"\n";
    }
    return ofs.good();
}

// 从 TOML 读取 CaseConfig，解析相对路径时以 TOML 所在目录为基准
bool read_case_toml(const std::string &path, CaseConfig &out) {
    auto map = toml_adapter::parse_toml_file(path);
    if (map.empty()) return false;
    out.case_path = path;
    auto get = [&](const std::string &k)->std::string{
        auto it = map.find(k);
        if (it == map.end()) return std::string();
        return it->second;
    };
    out.a_path = get("input.a.path");
    out.b_path = get("input.b.path");
    out.c_golden_path = get("output.c.golden");
    out.c_out_path = get("output.c.out");

    try {
        std::filesystem::path case_parent;
        try {
            std::filesystem::path cp(path);
            case_parent = cp.has_parent_path() ? cp.parent_path() : std::filesystem::current_path();
        } catch(...) { case_parent = std::filesystem::current_path(); }

        if (!out.a_path.empty()) {
            std::filesystem::path p(out.a_path);
            if (p.is_relative()) out.a_path = (case_parent / p).string();
        }
        if (!out.b_path.empty()) {
            std::filesystem::path p(out.b_path);
            if (p.is_relative()) out.b_path = (case_parent / p).string();
        }
        if (!out.c_golden_path.empty()) {
            std::filesystem::path p(out.c_golden_path);
            if (p.is_relative()) out.c_golden_path = (case_parent / p).string();
        }
        if (!out.c_out_path.empty()) {
            std::filesystem::path p(out.c_out_path);
            if (p.is_relative()) out.c_out_path = (case_parent / p).string();
        }
        out.model_cfg_path = get("model_cfg.path");
        if (!out.model_cfg_path.empty()) {
            std::filesystem::path p(out.model_cfg_path);
            if (p.is_relative()) out.model_cfg_path = (case_parent / p).string();
        }
        std::filesystem::path cp(path);
        out.case_path = std::filesystem::absolute(cp).string();
    } catch(...) {
        // 若解析失败则保留原始值
    }
    auto sa = get("input.a.addr"); if (!sa.empty()) out.a_addr = static_cast<uint32_t>(std::stoul(sa));
    auto sb = get("input.b.addr"); if (!sb.empty()) out.b_addr = static_cast<uint32_t>(std::stoul(sb));
    auto sc = get("output.c.addr"); if (!sc.empty()) out.c_addr = static_cast<uint32_t>(std::stoul(sc));
    auto ta = get("input.a.type"); if (!ta.empty()) out.a_type = ta;
    auto tb = get("input.b.type"); if (!tb.empty()) out.b_type = tb;
    auto tc = get("output.c.type"); if (!tc.empty()) out.c_type = tc;
    auto mm = get("meta.m"); if (!mm.empty()) out.M = std::stoi(mm);
    auto kk = get("meta.k"); if (!kk.empty()) out.K = std::stoi(kk);
    auto nn = get("meta.n"); if (!nn.empty()) out.N = std::stoi(nn);
    auto endian = get("meta.endian"); if (!endian.empty()) out.endian = endian;
    return true;
}

bool create_cube_case_config(const std::string &case_toml, CaseConfig &cfg,
                       const std::string &case_dir, const std::string &base_name,
                       const std::vector<int16_t> &A, const std::vector<int16_t> &B,
                       int M, int K, int N) {
    try {
        std::filesystem::create_directories(case_dir);
    } catch(...) {
        // 忽略创建失败
    }
    cfg.case_path = case_toml;
    cfg.a_path = case_dir + std::string("/") + base_name + std::string("_A.bin");
    cfg.b_path = case_dir + std::string("/") + base_name + std::string("_B.bin");
    cfg.c_golden_path = case_dir + std::string("/") + base_name + std::string("_C_golden.bin");
    cfg.c_out_path = case_dir + std::string("/") + base_name + std::string("_C_out.bin");
    cfg.a_addr = 0;
    cfg.b_addr = static_cast<uint32_t>(A.size());
    cfg.c_addr = static_cast<uint32_t>(A.size() + B.size());
    cfg.M = M; cfg.K = K; cfg.N = N;
    // 默认引用共享的 model_cfg.toml（相对路径）
    cfg.model_cfg_path = std::string("model_cfg.toml");

    if (!util::write_bin<int16_t>(cfg.a_path, A)) return false;
    if (!util::write_bin<int16_t>(cfg.b_path, B)) return false;

    std::vector<int32_t> Cgold(static_cast<size_t>(M) * static_cast<size_t>(N), 0);
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            int32_t acc = 0;
            for (int k = 0; k < K; ++k) {
                acc += static_cast<int32_t>(A[i*K + k]) * static_cast<int32_t>(B[k*N + j]);
            }
            Cgold[i* N + j] = acc;
        }
    }
    if (!util::write_bin<int32_t>(cfg.c_golden_path, Cgold)) return false;
    return write_case_toml(cfg);
}

// Read A/B binaries according to an already-populated CaseConfig
bool read_bins_from_cfg(const CaseConfig &cfg, std::vector<DataType> &A, std::vector<DataType> &B) {
    if (!read_bin<DataType>(cfg.a_path, A)) {
        std::cerr << "read_bins_from_cfg: failed to read A from " << cfg.a_path << std::endl;
        return false;
    }
    if (!read_bin<DataType>(cfg.b_path, B)) {
        std::cerr << "read_bins_from_cfg: failed to read B from " << cfg.b_path << std::endl;
        return false;
    }
    return true;
}

} // namespace util
