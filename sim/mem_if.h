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
    int max_outstanding;
    int issue_bw_read;
    int issue_bw_write;
    int complete_bw_read;
    int complete_bw_write;
    uint64_t current_cycle;
    int issued_read_this_cycle;
    int issued_write_this_cycle;
    struct Request {
        uint32_t addr;
        // shared pointer to a completion queue where completed data will be pushed
        std::shared_ptr<std::deque<DataType>> completion_queue;
        // maximum allowed elements in the completion queue before we consider it "full"
        size_t max_queue_depth;
        bool is_write;
        DataType write_data;
        int remaining_cycles;
        size_t len;      // burst length (for reads)
        size_t progress; // how many elements already produced
    };
    std::vector<Request> pending_requests;

public:
    MemoryInterface(int size_kb, int lat, int issue_bw, int complete_bw, int max_outstanding = -1);

    // 向 memory 发起读请求，完成后数据会被 push 到 completion_queue（遵守 max_queue_depth）
    // completion_queue is non-owning; caller must ensure it lives until request completes
    bool read_request(uint32_t addr, std::shared_ptr<std::deque<DataType>> completion_queue, size_t max_queue_depth = SIZE_MAX, size_t len = 1);
    bool write_request(uint32_t addr, DataType data);

    void cycle();  // 每个周期调用
    // 直接加载初始内存内容到内存模型（用于将 A/B 放入内存）
    void load_data(const std::vector<DataType>& data, uint32_t offset = 0);
    bool has_pending() const { return !pending_requests.empty(); }
};

#endif // MEMORY_INTERFACE_H
