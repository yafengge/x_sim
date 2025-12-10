#ifndef MEMORY_INTERFACE_H
#define MEMORY_INTERFACE_H

#include "systolic_common.h"
#include "fifo.h"
#include <vector>
#include <memory>
#include <deque>

class MemoryInterface {
private:
    std::vector<DataType> memory;
    int latency;
    int bandwidth;
    struct Request {
        uint32_t addr;
        // shared pointer to a completion queue where completed data will be pushed
        std::shared_ptr<std::deque<DataType>> completion_queue;
        // maximum allowed elements in the completion queue before we consider it "full"
        size_t max_queue_depth;
        bool is_write;
        DataType write_data;
        int remaining_cycles;
    };
    std::vector<Request> pending_requests;

public:
    MemoryInterface(int size_kb, int lat, int bw);

    // 向 memory 发起读请求，完成后数据会被 push 到 completion_queue（遵守 max_queue_depth）
    // completion_queue is non-owning; caller must ensure it lives until request completes
    bool read_request(uint32_t addr, std::shared_ptr<std::deque<DataType>> completion_queue, size_t max_queue_depth = SIZE_MAX);
    bool write_request(uint32_t addr, DataType data);

    void cycle();  // 每个周期调用
    // 直接加载初始内存内容到内存模型（用于将 A/B 放入内存）
    void load_data(const std::vector<DataType>& data, uint32_t offset = 0);
    bool has_pending() const { return !pending_requests.empty(); }
};

#endif // MEMORY_INTERFACE_H
