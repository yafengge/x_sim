#ifndef MINI_TOML_H
#define MINI_TOML_H

#include <string>
#include <unordered_map>

// 文件：config/mini_toml.h
// 说明：项目使用的极简 TOML 风格解析器接口，满足简单键值与表格读取需求。
// 支持特性：
//  - 简单的节（[section]）和嵌套节（[a.b]）
//  - 点分键（a.b = value）
//  - 带引号的字符串、整数、布尔值（true/false）
//  - 注释以 '#' 开头
// 说明：这不是完整的 TOML 实现，仅为项目配置读取提供轻量替代品。

std::unordered_map<std::string, std::string> parse_toml_file(const std::string& path);

#endif // MINI_TOML_H
