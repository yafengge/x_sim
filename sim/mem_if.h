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
    // PV write: 将宿主内存中的数据写入模拟内存。
    // dataAddr: 指向宿主内存中首元素的地址（按 DataType 计），
    // size: 元素数量（DataType 个数），
    // memAddr: 模拟内存中的目标地址（按元素索引计）。
    void pv_write(uint64_t dataAddr, size_t size, uint64_t memAddr);
    bool has_pending() const { return !pending_requests_.empty(); }
    // Expose configured latency for callers
    int get_latency() const { return latency_; }

    // Store accumulator (32-bit) values directly into an accumulator memory
    // region. These are synchronous helpers used by the Cube to commit results.
    void store_acc_direct(uint32_t addr, AccType val);

    // PV read: 从模拟累加器内存读取 `size` 个元素到宿主内存。
    // memAddr: 模拟内存地址（按元素索引）；
    // size: 元素数量；
    // dataAddr: 指向宿主内存目标缓冲区的地址（按 AccType 计）。
    bool pv_read(uint64_t memAddr, size_t size, uint64_t dataAddr) const;

private:
    // Load configuration values (reads the runtime default config path).
    void config();
};

#endif // MEMORY_INTERFACE_H
