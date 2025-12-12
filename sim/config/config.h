#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include <string>
#include <optional>
#include "types.h"
#include "toml_parser.h"

namespace config {

struct Config {
    int array_rows = 0;
    int array_cols = 0;
    int tile_rows = 0;
    int tile_cols = 0;
    Dataflow dataflow = Dataflow::WEIGHT_STATIONARY;

    // Parse from a flat map produced by TomlParser. Returns std::nullopt on error.
    static std::optional<Config> from_map(const TomlParser::map_t &m, std::string *err_msg = nullptr);
};

} // namespace config

#endif // CONFIG_CONFIG_H
