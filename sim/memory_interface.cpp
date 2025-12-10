#include "memory_interface.h"
#include <cstddef>
#include <memory>

MemoryInterface::MemoryInterface(int size_kb, int lat, int bw, int max_out)
    : latency(lat), bandwidth(bw) {
    if (max_out > 0) {
        max_outstanding = max_out;
    } else {
        // 简单估计：每周期带宽 * 延迟
        max_outstanding = bandwidth * latency;
    }
    // 简单地把 size_kb 解释为元素数量（而不是字节），以便测试时足够
    memory.resize(size_kb);
}

bool MemoryInterface::read_request(uint32_t addr, std::shared_ptr<std::deque<DataType>> completion_queue, size_t max_queue_depth) {
    if (addr >= memory.size()) return false;
    if (static_cast<int>(pending_requests.size()) >= max_outstanding) return false;
    Request req;
    req.addr = addr;
    req.completion_queue = completion_queue;
    req.max_queue_depth = max_queue_depth;
    req.is_write = false;
    req.remaining_cycles = latency;
    pending_requests.push_back(req);
    return true;
}

bool MemoryInterface::write_request(uint32_t addr, DataType data) {
    if (addr >= memory.size()) return false;
    if (static_cast<int>(pending_requests.size()) >= max_outstanding) return false;
    Request req;
    req.addr = addr;
    req.completion_queue = std::shared_ptr<std::deque<DataType>>();
    req.max_queue_depth = 0;
    req.is_write = true;
    req.write_data = data;
    req.remaining_cycles = latency;
    pending_requests.push_back(req);
    return true;
}

void MemoryInterface::cycle() {
    // 遍历 pending_requests，减少 remaining_cycles；当到达 0 时尝试完成请求（受带宽限制）
    // 我们尽量按队列顺序处理请求
    int completed_this_cycle = 0;
    for (auto it = pending_requests.begin(); it != pending_requests.end(); ) {
        if (it->remaining_cycles > 0) {
            it->remaining_cycles--;
            ++it;
            continue;
        }

        if (completed_this_cycle >= bandwidth) {
            // 本周期带宽已满，保留请求到下一周期
            ++it;
            continue;
        }

        if (it->is_write) {
            memory[it->addr] = it->write_data;
            it = pending_requests.erase(it);
            completed_this_cycle++;
            continue;
        } else {
            // 读请求：尝试把数据 push 到 completion_queue（如果给定）
            auto q = it->completion_queue;
            if (q) {
                if (q->size() < it->max_queue_depth) {
                    q->push_back(memory[it->addr]);
                    it = pending_requests.erase(it);
                    completed_this_cycle++;
                    continue;
                } else {
                    // completion queue 已满，保留请求
                    ++it;
                    continue;
                }
            } else {
                // 没有目标队列，丢弃或保留? 目前选择丢弃请求以防止死锁
                it = pending_requests.erase(it);
                completed_this_cycle++;
                continue;
            }
        }
    }
}

void MemoryInterface::load_data(const std::vector<DataType>& data, uint32_t offset) {
    if (offset + data.size() > memory.size()) {
        // 如果不足，扩展内存
        memory.resize(offset + data.size());
    }
    for (std::size_t i = 0; i < data.size(); ++i) {
        memory[offset + i] = data[i];
    }
}
