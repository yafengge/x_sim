#pragma once
#include <vector>
#include <string>
#include <fstream>

namespace util {

template<typename T>
inline bool write_bin(const std::string &path, const std::vector<T> &v) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    ofs.write(reinterpret_cast<const char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(T)));
    return ofs.good();
}

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

// Backwards-compatible wrappers
inline bool write_bin_int16(const std::string &path, const std::vector<int16_t> &v) { return write_bin<int16_t>(path, v); }
inline bool write_bin_int32(const std::string &path, const std::vector<int32_t> &v) { return write_bin<int32_t>(path, v); }
inline bool read_bin_int16(const std::string &path, std::vector<int16_t> &v) { return read_bin<int16_t>(path, v); }
inline bool read_bin_int32(const std::string &path, std::vector<int32_t> &v) { return read_bin<int32_t>(path, v); }

} // namespace util
