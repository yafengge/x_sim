#pragma once
#include <vector>
#include <string>
#include "util/case_io.h"
#include <optional>
#include <filesystem>
#include <fstream>

// 文件：util/utils.h
// 说明：通用轻量工具集合，包含：
// - header-only 二进制读写模板 `write_bin` / `read_bin`（用于序列化矩阵/缓冲区）
// - 路径查找/解析辅助（`find_existing_path` / `resolve_path`）
// - 同目录下的验证工具在 `util/verify.h` / `util/verify.cpp` 中实现
#include "util/verify.h"

namespace util {

// 使用示例：`util::write_bin<int16_t>(path, vec)` / `util::read_bin<int16_t>(path, vec)`

// 二进制写入模板：将 vector 原始内存以二进制形式写入文件。
template<typename T>
inline bool write_bin(const std::string &path, const std::vector<T> &v) {
	std::ofstream ofs(path, std::ios::binary);
	if (!ofs) return false;
	ofs.write(reinterpret_cast<const char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(T)));
	return ofs.good();
}

// 二进制读取模板：读取整个文件为 vector（要求文件大小为 T 的整数倍）。
template<typename T>
inline bool read_bin(const std::string &path, std::vector<T> &v) {
	std::ifstream ifs(path, std::ios::binary | std::ios::ate);
	if (!ifs) return false;
	std::streamsize sz = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	if (sz % sizeof(T) != 0) return false;
	size_t count = static_cast<size_t>(sz / sizeof(T));
	v.resize(count);
	ifs.read(reinterpret_cast<char*>(v.data()), sz);
	return ifs.good();
}

// 路径辅助函数：在当前目录及向上查找 `path`，用于发现相对文件或资源。
std::optional<std::filesystem::path> find_existing_path(const std::string &path, int max_up = 5);
// 将给定路径解析为绝对路径（若已是绝对路径则直接返回）。
std::string resolve_path(const std::string &path);

// 注意：Per-case TOML 接口在 `util::case_io.h`，此头文件包含该声明以便统一使用。

} // namespace util
