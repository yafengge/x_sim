#include "processing_element.h"
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
    pipeline_reg.valid = false;
}

void ProcessingElement::load_weight(DataType w) {
    weight = w;
    weight_valid = true;
    active = true;
}

void ProcessingElement::compute_cycle(DataType act_in, AccType psum_in,
                                      DataType& act_out, AccType& psum_out) {
    // 传递并转发激活值（右移）：当前周期应将输入激活向右传递
    act_out = act_in;
    activation = act_in;

    // 正确的MAC计算：使用传入的激活值与本PE的权重相乘
    AccType mac_result = 0;
    if (weight_valid && activation != 0) {
        mac_result = static_cast<AccType>(activation) * 
                     static_cast<AccType>(weight);
    }

    // 计算新的部分和并返回给调用者（不在此直接写回累加器，
    // 由上层在同步阶段一次性写回以保证时序一致性）
    AccType new_psum = psum_in + mac_result;
    psum_out = new_psum;

    // 更新流水线寄存器
    pipeline_reg.activation = act_in;
    pipeline_reg.partial_sum = new_psum;
    pipeline_reg.valid = (weight_valid && activation != 0);
}

void ProcessingElement::print_state() const {
    std::cout << "PE(" << id_x << "," << id_y << "): "
              << "W=" << weight << " A=" << activation 
              << " ACC=" << accumulator 
              << " " << (weight_valid ? "W" : "-")
              << (active ? "A" : "-") << std::endl;
}
