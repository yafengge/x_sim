#ifndef SYSTOLIC_COMMON_H
#define SYSTOLIC_COMMON_H

#include <cstdint>

// 数据类型定义
typedef int16_t DataType;      // 16位定点数
typedef int32_t AccType;       // 32位累加器
typedef uint64_t Cycle;        // 周期计数器

// 脉动阵列配置
struct SystolicConfig {
    int array_rows;      // 阵列行数
    int array_cols;      // 阵列列数
    int pe_latency;      // PE计算延迟（周期）
    int memory_latency;  // 内存访问延迟
    int bandwidth;       // 带宽（字/周期）
    int progress_interval; // If >0, print a progress update every this many tiles
    bool verbose;          // Verbose printing flag

    // 数据流模式
    enum class Dataflow {
        WEIGHT_STATIONARY,   // 权重固定（最常用）
        OUTPUT_STATIONARY,   // 输出固定
        INPUT_STATIONARY     // 输入固定
    };

    Dataflow dataflow;

    SystolicConfig(int r=8, int c=8) : 
        array_rows(r), array_cols(c), pe_latency(1), 
        memory_latency(10), bandwidth(4), 
        progress_interval(0), verbose(false),
        dataflow(Dataflow::WEIGHT_STATIONARY) {}
};

#endif // SYSTOLIC_COMMON_H
