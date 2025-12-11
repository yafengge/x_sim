#include "mem_if.h"
#include "config/config_mgr.h"

#include <cstddef>
#include <memory>

// mem_if.cpp — 内存模型实现（中文注释）
// 实现 `Mem` 类的异步读/写请求队列、按周期推进的完成逻辑以及数据加载方法。
// 该实现模拟带宽与延迟、突发读写并为上层提供完成队列回调风格的接口。

Mem::Mem(int size_kb, int lat, int issue_bw, int complete_bw, int max_out)
    : latency(lat),
      max_outstanding(max_out),
      issue_bw_read(issue_bw), issue_bw_write(issue_bw),
      complete_bw_read(complete_bw), complete_bw_write(complete_bw),
      current_cycle(0), issued_read_this_cycle(0), issued_write_this_cycle(0) {
    // If caller passed <= 0 for max_out, estimate from complete bandwidth and latency
    if (max_outstanding <= 0) {
        max_outstanding = complete_bw * latency;
    }
    // Treat size_kb as number of elements for simplicity in tests
    memory.resize(size_kb);
}



bool Mem::read_request(uint32_t addr, std::shared_ptr<std::deque<DataType>> completion_queue, size_t max_queue_depth, size_t len) {
    if (addr >= memory.size()) return false;
    if (len == 0) return true;
    if (static_cast<int>(pending_requests.size()) >= max_outstanding) return false;
    if (issued_read_this_cycle >= issue_bw_read) return false;
    Request req;
    req.addr = addr;
    req.completion_queue = completion_queue;
    req.max_queue_depth = max_queue_depth;
    req.is_write = false;
    req.remaining_cycles = latency;
    req.len = len;
    req.progress = 0;
    pending_requests.push_back(req);
    issued_read_this_cycle++;
    return true;
}

bool Mem::write_request(uint32_t addr, DataType data) {
    if (addr >= memory.size()) return false;
    if (static_cast<int>(pending_requests.size()) >= max_outstanding) return false;
    if (issued_write_this_cycle >= issue_bw_write) return false;
    Request req;
    req.addr = addr;
    req.completion_queue = std::shared_ptr<std::deque<DataType>>();
    req.max_queue_depth = 0;
    req.is_write = true;
    req.write_data = data;
    req.remaining_cycles = latency;
    req.len = 1;
    req.progress = 0;
    pending_requests.push_back(req);
    issued_write_this_cycle++;
    return true;
}

void Mem::cycle() {
    current_cycle++;
    issued_read_this_cycle = 0;
    issued_write_this_cycle = 0;

    // 遍历 pending_requests，减少 remaining_cycles；当到达 0 时尝试完成请求（受带宽限制）
    int completed_read = 0;
    int completed_write = 0;
    for (auto it = pending_requests.begin(); it != pending_requests.end(); ) {
        if (it->remaining_cycles > 0) {
            it->remaining_cycles--;
            ++it;
            continue;
        }

        if (it->is_write) {
            if (completed_write >= complete_bw_write) { ++it; continue; }
            memory[it->addr] = it->write_data;
            it = pending_requests.erase(it);
            completed_write++;
            continue;
        }

        // 读请求完成：尝试按 burst 推送数据到 completion_queue
        if (completed_read >= complete_bw_read) { ++it; continue; }

        auto q = it->completion_queue;
        if (!q) {
            it = pending_requests.erase(it);
            completed_read++;
            continue;
        }

        size_t remaining_len = it->len - it->progress;
        size_t can_complete = std::min(static_cast<size_t>(complete_bw_read - completed_read), remaining_len);
        size_t pushed = 0;
        while (pushed < can_complete && q->size() < it->max_queue_depth) {
            uint32_t addr = it->addr + static_cast<uint32_t>(it->progress + pushed);
            if (addr >= memory.size()) break;
            q->push_back(memory[addr]);
            pushed++;
        }

        it->progress += pushed;
        completed_read += static_cast<int>(pushed);

        if (it->progress >= it->len) {
            it = pending_requests.erase(it);
            continue;
        }

        ++it;
    }
}

void Mem::load_data(const std::vector<DataType>& data, uint32_t offset) {
    if (offset + data.size() > memory.size()) {
        // 如果不足，扩展内存
        memory.resize(offset + data.size());
    }
    for (std::size_t i = 0; i < data.size(); ++i) {
        memory[offset + i] = data[i];
    }
}
