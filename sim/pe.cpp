#// 文件：pe.cpp
#// 说明：PE 实现文件。
#// 实现处理单元的本地乘加逻辑、流水线寄存器与两阶段提交（tick/commit）机制，
#// 主要函数包括 `reset`、`load_weight`、`prepare_inputs`、`tick` 和 `commit`。
#include "pe.h"
#include <iostream>
PE::PE(int x, int y) 
    : id_x(x), id_y(y), weight(0), activation(0), 
      accumulator(0), weight_valid(false), active(false) {
    reset();
}

// 将 PE 状态复位到初始值（在新的仿真/Tile 开始前使用）
void PE::reset() {
    weight = 0;
    activation = 0;
    accumulator = 0;
    weight_valid = false;
    active = false;
    pipeline_reg.activation = 0;
    pipeline_reg.partial_sum = 0;
    pipeline_reg.staged_weight = 0;
    pipeline_reg.staged_weight_valid = false;
}

// 加载权重到 PE（通常由阵列控制器广播）
void PE::load_weight(DataType w) {
    weight = w;
    weight_valid = true;
    active = true;
}

// 为基于时钟的执行准备输入（不立即更新可见寄存器）
void PE::prepare_inputs(DataType act_in, AccType psum_in, DataType weight_in, bool weight_present) {
    pipeline_reg.activation = act_in;
    pipeline_reg.partial_sum = psum_in;
    pipeline_reg.staged_weight = weight_in;
    pipeline_reg.staged_weight_valid = weight_present;
}

// 在 tick 时执行一次计算周期：使用已阶段化的输入更新流水线寄存器
void PE::tick() {
    DataType act_out;
    AccType psum_out;
    compute_cycle(pipeline_reg.activation, pipeline_reg.partial_sum, act_out, psum_out);
}

// 两阶段提交：把流水线寄存器的值一次性写回到可见寄存器，保证同步语义
void PE::commit() {
    activation = pipeline_reg.activation;
    accumulator = pipeline_reg.partial_sum;
    if (pipeline_reg.staged_weight_valid) {
        weight = pipeline_reg.staged_weight;
        weight_valid = true;
        pipeline_reg.staged_weight_valid = false;
        pipeline_reg.staged_weight = 0;
    }
}

// 单个周期内的计算逻辑（不直接写回可见寄存器）
void PE::compute_cycle(DataType act_in, AccType psum_in,
                                      DataType& act_out, AccType& psum_out) {
    act_out = act_in;
    AccType mac_result = 0;
    DataType used_weight = pipeline_reg.staged_weight_valid ? pipeline_reg.staged_weight : weight;
    bool used_weight_valid = pipeline_reg.staged_weight_valid ? true : weight_valid;
    if (used_weight_valid && act_in != 0) {
        mac_result = static_cast<AccType>(act_in) * static_cast<AccType>(used_weight);
    }
    AccType new_psum = psum_in + mac_result;
    psum_out = new_psum;
    pipeline_reg.activation = act_in;
    pipeline_reg.partial_sum = new_psum;
}

// 调试用：打印 PE 状态
void PE::print_state() const {
    std::cout << "PE(" << id_x << "," << id_y << "): "
              << "W=" << weight << " A=" << activation 
              << " ACC=" << accumulator 
              << " " << (weight_valid ? "W" : "-")
              << (active ? "A" : "-") << std::endl;
}
