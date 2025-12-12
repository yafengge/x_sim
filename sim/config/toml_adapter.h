#ifndef TOML_ADAPTER_H
#define TOML_ADAPTER_H

#include <string>
#include <unordered_map>

// 文件：config/toml_adapter.h
// 说明：TOML 解析器适配层。若启用第三方 toml++（定义 HAVE_TOMLPP），
// 则使用 toml++ 进行解析并扁平化为 dotted-key map；否则回退到项目内置的
// `mini_toml` 实现。这样可以逐步迁移而不引入立即的编译依赖。

namespace toml_adapter {

std::unordered_map<std::string, std::string> parse_toml_file(const std::string& path);

} // namespace toml_adapter

#endif // TOML_ADAPTER_H
