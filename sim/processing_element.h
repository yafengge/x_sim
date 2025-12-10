#ifndef PROCESSING_ELEMENT_H
#define PROCESSING_ELEMENT_H

#include "systolic_common.h"

class ProcessingElement {
private:
    int id_x, id_y;           // PE坐标
    DataType weight;          // 存储的权重
    DataType activation;      // 当前激活值
    AccType accumulator;      // 累加器
    bool weight_valid;        // 权重是否有效
    bool active;              // PE是否激活

    // 流水线寄存器
    struct {
        DataType activation;
        AccType partial_sum;
        bool valid;
        // staged weight for two-phase weight propagation
        DataType staged_weight;
        bool staged_weight_valid;
    } pipeline_reg;

public:
    ProcessingElement() = default;       

    ProcessingElement(int x, int y);
    
    // 重置PE
    void reset();
    
    // 加载权重
    void load_weight(DataType w);
    
    // 执行一个计算周期
    void compute_cycle(DataType act_in, AccType psum_in,
                       DataType& act_out, AccType& psum_out);

    // New: prepare inputs for a tick-driven compute and perform a tick
    void prepare_inputs(DataType act_in, AccType psum_in, DataType weight_in, bool weight_present);
    void tick();
    // Commit the computed next-state into visible registers (two-phase commit)
    void commit();
    // weight accessors for tick-driven mode
    DataType get_weight() const { return weight; }
    void set_weight(DataType w) { weight = w; weight_valid = true; }
    
    // 获取当前累加值
    AccType get_accumulator() const { return accumulator; }
    // 激活值访问器（供外部PE间通信使用）
    DataType get_activation() const { return activation; }
    void set_activation(DataType act) { activation = act; }
    // 累加器写入器（供外部PE间通信使用）
    void set_accumulator(AccType acc) { accumulator = acc; }
    
    // PE状态查询
    bool is_active() const { return active; }
    bool has_weight() const { return weight_valid; }
    
    // 打印状态
    void print_state() const;
};

#endif // PROCESSING_ELEMENT_H
