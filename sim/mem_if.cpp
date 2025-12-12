#include "mem_if.h"
#include "config/config.h"

#include <cstddef>
#include <memory>

// mem_if.cpp — 内存模型实现（中文注释）
// 实现 `Mem` 类的异步读/写请求队列、按周期推进的完成逻辑以及数据加载方法。
// 该实现模拟带宽与延迟、突发读写并为上层提供完成队列回调风格的接口。

Mem::Mem(p_clock_t clock)
        : latency_(10),
          max_outstanding_(0),
          issue_bw_read_(4), issue_bw_write_(4),
          complete_bw_read_(4), complete_bw_write_(4),
          current_cycle_(0), issued_read_this_cycle_(0), issued_write_this_cycle_(0) {
    // Centralize configuration reads
    config();
}

void Mem::config() {
    // Configuration is read via the runtime default path (set with config::set_default_path).
    std::string path = config::get_default_path();

    auto v_latency = get<int>("memory.memory_latency");
    latency_ = v_latency.value_or(10);

    auto v_bw = get<int>("memory.bandwidth");
    int bw = v_bw.value_or(4);
    issue_bw_read_ = issue_bw_write_ = bw;
    complete_bw_read_ = complete_bw_write_ = bw;

    auto v_maxout = get<int>("memory.max_outstanding");
    if (v_maxout.has_value() && v_maxout.value() > 0) {
        max_outstanding_ = v_maxout.value();
    } else {
        max_outstanding_ = complete_bw_read_ * latency_;
    }

    // memory size: default to 64 elements (previously 64 KB interpreted as elements)
    int size_kb = get<int>("memory.size_kb").value_or(64);
    memory_.resize(size_kb);

    // ensure accumulator memory is at least the same size (one-to-one mapping)
    acc_memory_.resize(memory_.size());
}



bool Mem::read_request(uint32_t addr, std::shared_ptr<std::deque<DataType>> completion_queue, size_t max_queue_depth, size_t len) {
    if (addr >= memory_.size()) return false;
    if (len == 0) return true;
    if (static_cast<int>(pending_requests_.size()) >= max_outstanding_) return false;
    if (issued_read_this_cycle_ >= issue_bw_read_) return false;
    Request req;
    req.addr = addr;
    req.completion_queue = completion_queue;
    req.max_queue_depth = max_queue_depth;
    req.is_write = false;
    req.remaining_cycles = latency_;
    req.len = len;
    req.progress = 0;
    pending_requests_.push_back(req);
    issued_read_this_cycle_++;
    return true;
}

bool Mem::write_request(uint32_t addr, DataType data) {
    if (addr >= memory_.size()) return false;
    if (static_cast<int>(pending_requests_.size()) >= max_outstanding_) return false;
    if (issued_write_this_cycle_ >= issue_bw_write_) return false;
    Request req;
    req.addr = addr;
    req.completion_queue = std::shared_ptr<std::deque<DataType>>();
    req.max_queue_depth = 0;
    req.is_write = true;
    req.write_data = data;
    req.remaining_cycles = latency_;
    req.len = 1;
    req.progress = 0;
    pending_requests_.push_back(req);
    issued_write_this_cycle_++;
    return true;
}

void Mem::cycle() {
    current_cycle_++;
    issued_read_this_cycle_ = 0;
    issued_write_this_cycle_ = 0;

    // 遍历 pending_requests，减少 remaining_cycles；当到达 0 时尝试完成请求（受带宽限制）
    int completed_read = 0;
    int completed_write = 0;
    for (auto it = pending_requests_.begin(); it != pending_requests_.end(); ) {
        if (it->remaining_cycles > 0) {
            it->remaining_cycles--;
            ++it;
            continue;
        }

        if (it->is_write) {
            if (completed_write >= complete_bw_write_) { ++it; continue; }
            memory_[it->addr] = it->write_data;
            it = pending_requests_.erase(it);
            completed_write++;
            continue;
        }

        // 读请求完成：尝试按 burst 推送数据到 completion_queue
        if (completed_read >= complete_bw_read_) { ++it; continue; }

        auto q = it->completion_queue;
        if (!q) {
            it = pending_requests_.erase(it);
            completed_read++;
            continue;
        }

        size_t remaining_len = it->len - it->progress;
        size_t can_complete = std::min(static_cast<size_t>(complete_bw_read_ - completed_read), remaining_len);
        size_t pushed = 0;
        while (pushed < can_complete && q->size() < it->max_queue_depth) {
            uint32_t addr = it->addr + static_cast<uint32_t>(it->progress + pushed);
            if (addr >= memory_.size()) break;
            q->push_back(memory_[addr]);
            pushed++;
        }

        it->progress += pushed;
        completed_read += static_cast<int>(pushed);

        if (it->progress >= it->len) {
            it = pending_requests_.erase(it);
            continue;
        }

        ++it;
    }
}

void Mem::pv_write(uint64_t dataAddr, size_t size, uint64_t memAddr) {
    // Interpret dataAddr as pointer to DataType elements in host memory.
    const DataType* src = reinterpret_cast<const DataType*>(static_cast<uintptr_t>(dataAddr));
    uint64_t offset = memAddr;
    if (offset + size > memory_.size()) {
        memory_.resize(static_cast<size_t>(offset + size));
    }
    for (size_t i = 0; i < size; ++i) {
        memory_[static_cast<size_t>(offset + i)] = src[i];
    }
}

void Mem::store_acc_direct(uint32_t addr, AccType val) {
    if (addr >= acc_memory_.size()) {
        // expand accumulator storage if needed
        acc_memory_.resize(addr + 1);
    }
    acc_memory_[addr] += val;
}

bool Mem::pv_read(uint64_t memAddr, size_t size, uint64_t dataAddr) const {
    if (size == 0) return true;
    uint64_t addr = memAddr;
    if (addr + size > acc_memory_.size()) return false;
    AccType* dst = reinterpret_cast<AccType*>(static_cast<uintptr_t>(dataAddr));
    for (size_t i = 0; i < size; ++i) dst[i] = acc_memory_[static_cast<size_t>(addr + i)];
    return true;
}
