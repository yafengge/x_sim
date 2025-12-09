#ifndef MEMORY_INTERFACE_H
#define MEMORY_INTERFACE_H

#include "systolic_common.h"
#include "fifo.h"
#include <vector>

class MemoryInterface {
private:
    std::vector<DataType> memory;
    int latency;
    int bandwidth;
    struct Request {
        uint32_t addr;
        FIFO* dest; // 目标 FIFO，当请求完成时将数据 push 到该 FIFO
        bool is_write;
        DataType write_data;
        int remaining_cycles;
    };
    std::vector<Request> pending_requests;

public:
    MemoryInterface(int size_kb, int lat, int bw);

    // 向 memory 发起读请求，完成后数据会被 push 到 dest FIFO（遵守 FIFO 深度）
    bool read_request(uint32_t addr, FIFO* dest);
    bool write_request(uint32_t addr, DataType data);

    void cycle();  // 每个周期调用
    // 直接加载初始内存内容到内存模型（用于将 A/B 放入内存）
    void load_data(const std::vector<DataType>& data, uint32_t offset = 0);
    bool has_pending() const { return !pending_requests.empty(); }
};

#endif // MEMORY_INTERFACE_H
