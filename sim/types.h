#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <memory>

// Forward declarations for shared pointer aliases
class Clock;
class Mem;
class Cube;
class SystolicArray;

// 数据类型定义
typedef int16_t DataType;      // 16位定点数
typedef int32_t AccType;       // 32位累加器
typedef uint64_t Cycle;        // 周期计数器

// Configuration types: only keep Dataflow enum here.  Other config
// is read on-demand via `config_loader` per-key getters; the in-memory
// CubeConfig/MemConfig structs have been removed.
enum class Dataflow {
    WEIGHT_STATIONARY,
    OUTPUT_STATIONARY,
    INPUT_STATIONARY
};

// Common pointer aliases
using p_clock_t = std::shared_ptr<Clock>;
using p_mem_t = std::shared_ptr<Mem>;
using p_cube_t = std::shared_ptr<Cube>;
using p_systolic_array_t = std::shared_ptr<SystolicArray>;

#endif // TYPES_H
