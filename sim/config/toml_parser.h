#ifndef CONFIG_TOML_PARSER_H
#define CONFIG_TOML_PARSER_H

#include <string>
#include <unordered_map>

namespace config {

class TomlParser {
public:
    using map_t = std::unordered_map<std::string, std::string>;
    // Parse a TOML file and return a flat dotted-key -> string map (keys normalized to lower-case)
    static map_t parse_file(const std::string &path) noexcept;
};

} // namespace config

#endif // CONFIG_TOML_PARSER_H
