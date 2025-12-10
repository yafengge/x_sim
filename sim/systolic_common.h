#ifndef SYSTOLIC_COMMON_H
#define SYSTOLIC_COMMON_H

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

// Configuration types split between cube-level and memory-level
enum class Dataflow {
    WEIGHT_STATIONARY,
    OUTPUT_STATIONARY,
    INPUT_STATIONARY
};

struct CubeConfig {
    int array_rows;      // 阵列行数
    int array_cols;      // 阵列列数
    int pe_latency;      // PE计算延迟（周期）
    int progress_interval; // If >0, print a progress update every this many tiles
    bool verbose;          // Verbose printing flag
    int trace_cycles;      // 如 >0，对前 trace_cycles 周期打印跟踪
    Dataflow dataflow;

    CubeConfig(int r=8, int c=8)
      : array_rows(r), array_cols(c), pe_latency(1),
        progress_interval(0), verbose(false), trace_cycles(0),
        dataflow(Dataflow::WEIGHT_STATIONARY) {}
};

struct MemConfig {
    int memory_latency;  // 内存访问延迟
    int bandwidth;       // 带宽（字/周期）
    int max_outstanding; // 内存最大未完成请求数（<=0 表示按带宽*延迟自动推算）

    MemConfig() : memory_latency(10), bandwidth(4), max_outstanding(0) {}
};

// Common pointer aliases
using p_clock_t = std::shared_ptr<Clock>;
using p_mem_t = std::shared_ptr<Mem>;
using p_cube_t = std::shared_ptr<Cube>;
using p_systolic_array_t = std::shared_ptr<SystolicArray>;

#endif // SYSTOLIC_COMMON_H
