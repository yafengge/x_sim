// mem_if.h — 内存接口（中文注释）
// 定义了仿真中的内存模型接口 `Mem`，支持异步读写请求、突发读取、
// 完成队列（completion queue）以及按周期推进的行为。外部驱动通过
// 提交请求并在完成队列中读取数据来与内存模型交互。
#ifndef MEMORY_INTERFACE_H
#define MEMORY_INTERFACE_H

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "types.h"
#include "fifo.h"

class Clock;

class Mem {
private:
    std::vector<DataType> memory_;
    // accumulator memory region (stores 32-bit accumulator results)
    std::vector<AccType> acc_memory_;
    int latency_;
    int max_outstanding_;
    int issue_bw_read_;
    int issue_bw_write_;
    int complete_bw_read_;
    int complete_bw_write_;
    uint64_t current_cycle_;
    int issued_read_this_cycle_;
    int issued_write_this_cycle_;
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
    std::vector<Request> pending_requests_;

public:
    // New constructor: parameters are read from the configuration file. If
    // `config_path` is empty the constructor will probe common locations
    // (`model_cfg.toml`, `config/model.toml`). The `clock` parameter is
    // optional and currently unused by the memory model itself.
    Mem(p_clock_t clock = nullptr);

    // Configuration is provided via per-key getters.

    // 向 memory 发起读请求，完成后数据会被 push 到 completion_queue（遵守 max_queue_depth）
    // completion_queue is non-owning; caller must ensure it lives until request completes
    bool read_request(uint32_t addr, std::shared_ptr<std::deque<DataType>> completion_queue, size_t max_queue_depth = SIZE_MAX, size_t len = 1);
    bool write_request(uint32_t addr, DataType data);

    void cycle();  // 每个周期调用
    // 直接加载初始内存内容到内存模型（用于将 A/B 放入内存）
    void load_data(const std::vector<DataType>& data, uint32_t offset = 0);
    bool has_pending() const { return !pending_requests_.empty(); }
    // Expose configured latency for callers
    int get_latency() const { return latency_; }

    // Store accumulator (32-bit) values directly into an accumulator memory
    // region. These are synchronous helpers used by the Cube to commit results.
    void store_acc_direct(uint32_t addr, AccType val);

    // Dump a contiguous range of accumulator values into `out` (returns true on success)
    bool dump_acc(uint32_t addr, size_t len, std::vector<AccType>& out) const;

private:
    // Load configuration values (reads the runtime default config path).
    void config();
};

#endif // MEMORY_INTERFACE_H
