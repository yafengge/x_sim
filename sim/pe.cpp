#include "pe.h"
#include <iostream>

ProcessingElement::ProcessingElement(int x, int y) 
    : id_x(x), id_y(y), weight(0), activation(0), 
      accumulator(0), weight_valid(false), active(false) {
    reset();
}

void ProcessingElement::reset() {
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

void ProcessingElement::load_weight(DataType w) {
    weight = w;
    weight_valid = true;
    active = true;
}

// Prepare inputs for tick-driven execution
void ProcessingElement::prepare_inputs(DataType act_in, AccType psum_in, DataType weight_in, bool weight_present) {
    // stage inputs into pipeline registers (do not update visible registers yet)
    pipeline_reg.activation = act_in;
    pipeline_reg.partial_sum = psum_in;
    // stage weight for this cycle; actual update happens in commit(); record presence
    pipeline_reg.staged_weight = weight_in;
    pipeline_reg.staged_weight_valid = weight_present;
}

// Execute one tick: perform compute based on prepared inputs
void ProcessingElement::tick() {
    DataType act_out;
    AccType psum_out;
    // use staged inputs; compute and update pipeline_reg but do not make values visible
    compute_cycle(pipeline_reg.activation, pipeline_reg.partial_sum, act_out, psum_out);
}

// Commit the staged next-state into visible registers (two-phase commit)
void ProcessingElement::commit() {
    // Always commit staged activation/psum to make sure zeros propagate and
    // accumulators do not stick across idle cycles.
    activation = pipeline_reg.activation;
    accumulator = pipeline_reg.partial_sum;
    // apply staged weight if present
    if (pipeline_reg.staged_weight_valid) {
        weight = pipeline_reg.staged_weight;
        weight_valid = true;
        pipeline_reg.staged_weight_valid = false;
        pipeline_reg.staged_weight = 0;
    }
}

void ProcessingElement::compute_cycle(DataType act_in, AccType psum_in,
                                      DataType& act_out, AccType& psum_out) {
    // 传递并转发激活值（右移）：当前周期应将输入激活向右传递
    act_out = act_in;

    // 正确的MAC计算：优先使用本周期阶段性提供的权重（staged），否则使用可见权重
    AccType mac_result = 0;
    DataType used_weight = pipeline_reg.staged_weight_valid ? pipeline_reg.staged_weight : weight;
    bool used_weight_valid = pipeline_reg.staged_weight_valid ? true : weight_valid;
    if (used_weight_valid && act_in != 0) {
        mac_result = static_cast<AccType>(act_in) * static_cast<AccType>(used_weight);
    }

    // 计算新的部分和并返回给调用者（不在此直接写回累加器，
    // 由上层在同步阶段一次性写回以保证时序一致性）
    AccType new_psum = psum_in + mac_result;
    psum_out = new_psum;

    // 更新流水线寄存器（不要直接更新可见寄存器，这将在 commit 阶段进行）
    pipeline_reg.activation = act_in;
    pipeline_reg.partial_sum = new_psum;
    // keep the valid bit set by prepare_inputs so commit always runs; do not
    // gate commits on non-zero activations to avoid stuck activations.
}

void ProcessingElement::print_state() const {
    std::cout << "PE(" << id_x << "," << id_y << "): "
              << "W=" << weight << " A=" << activation 
              << " ACC=" << accumulator 
              << " " << (weight_valid ? "W" : "-")
              << (active ? "A" : "-") << std::endl;
}
