#include "config_loader.h"
#include <algorithm>
#include <cctype>

static std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return s;
}

bool get_int_key(const std::string& path, const std::string& dotted_key, int& out) {
    return ConfigCache::instance().get_int(path, dotted_key, out);
}

bool get_bool_key(const std::string& path, const std::string& dotted_key, bool& out) {
    return ConfigCache::instance().get_bool(path, dotted_key, out);
}

bool get_string_key(const std::string& path, const std::string& dotted_key, std::string& out) {
    return ConfigCache::instance().get_string(path, dotted_key, out);
}

bool get_dataflow_key(const std::string& path, const std::string& dotted_key, Dataflow& out) {
    std::string v;
    if (!ConfigCache::instance().get_dataflow(path, dotted_key, v)) return false;
    v = to_upper(v);
    if (v == "WEIGHT_STATIONARY") { out = Dataflow::WEIGHT_STATIONARY; return true; }
    if (v == "OUTPUT_STATIONARY") { out = Dataflow::OUTPUT_STATIONARY; return true; }
    if (v == "INPUT_STATIONARY") { out = Dataflow::INPUT_STATIONARY; return true; }
    return false;
}
