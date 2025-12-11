#include "mem_if.h"
#include "config/config_mgr.h"

#include <cstddef>
#include <memory>

// mem_if.cpp — 内存模型实现（中文注释）
// 实现 `Mem` 类的异步读/写请求队列、按周期推进的完成逻辑以及数据加载方法。
// 该实现模拟带宽与延迟、突发读写并为上层提供完成队列回调风格的接口。

Mem::Mem(p_clock_t /*clock*/, const std::string &config_path)
        : latency_(10),
          max_outstanding_(0),
          issue_bw_read_(4), issue_bw_write_(4),
          complete_bw_read_(4), complete_bw_write_(4),
          current_cycle_(0), issued_read_this_cycle_(0), issued_write_this_cycle_(0) {
    // Centralize configuration reads
    config(config_path);
}

void Mem::config(const std::string &config_path) {
    // Determine config file to read. If the caller provided a path use it,
    // otherwise try common locations.
    std::string path = config_path;
    if (path.empty()) {
        path = "model_cfg.toml";
    }

    int tmp_int = 0;
    if (!get_int_key(path, "memory.memory_latency", tmp_int)) tmp_int = 10;
    latency_ = tmp_int;

    if (!get_int_key(path, "memory.bandwidth", tmp_int)) tmp_int = 4;
    issue_bw_read_ = issue_bw_write_ = tmp_int;
    complete_bw_read_ = complete_bw_write_ = tmp_int;

    if (!get_int_key(path, "memory.max_outstanding", tmp_int)) tmp_int = 0;
    if (tmp_int > 0) {
        max_outstanding_ = tmp_int;
    } else {
        max_outstanding_ = complete_bw_read_ * latency_;
    }

    // memory size: default to 64 elements (previously 64 KB interpreted as elements)
    int size_kb = 64;
    if (!get_int_key(path, "memory.size_kb", size_kb)) {
        // no explicit size configured, keep default
    }
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

void Mem::load_data(const std::vector<DataType>& data, uint32_t offset) {
    if (offset + data.size() > memory_.size()) {
        // 如果不足，扩展内存
        memory_.resize(offset + data.size());
    }
    for (std::size_t i = 0; i < data.size(); ++i) {
        memory_[offset + i] = data[i];
    }
}

void Mem::store_acc_direct(uint32_t addr, AccType val) {
    if (addr >= acc_memory_.size()) {
        // expand accumulator storage if needed
        acc_memory_.resize(addr + 1);
    }
    acc_memory_[addr] += val;
}

bool Mem::dump_acc(uint32_t addr, size_t len, std::vector<AccType>& out) const {
    if (len == 0) { out.clear(); return true; }
    if (addr + len > acc_memory_.size()) return false;
    out.resize(len);
    for (size_t i = 0; i < len; ++i) out[i] = acc_memory_[addr + i];
    return true;
}
