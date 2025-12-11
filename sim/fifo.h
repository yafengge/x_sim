// fifo.h — 简单环形 FIFO（中文注释）
// 本文件实现了用于 PE/阵列内部的轻量环形 FIFO 结构，支持基本的 push/pop 操作
// 以及重置操作。该实现为仿真提供简洁的队列语义。
#ifndef SYSTOLIC_FIFO_H
#define SYSTOLIC_FIFO_H

#include "types.h"
#include <vector>

struct FIFO {
    std::vector<DataType> buffer;
    int depth;
    int read_ptr, write_ptr;
    int count;

    FIFO(int d=1) : depth(d), read_ptr(0), write_ptr(0), count(0) {
        buffer.resize(d);
    }

    bool push(DataType data) {
        if (count >= depth) return false;
        buffer[write_ptr] = data;
        write_ptr = (write_ptr + 1) % depth;
        count++;
        return true;
    }

    bool pop(DataType& data) {
        if (count <= 0) return false;
        data = buffer[read_ptr];
        read_ptr = (read_ptr + 1) % depth;
        count--;
        return true;
    }

    // Reset FIFO depth and clear pointers/count.
    void reset(int new_depth) {
        depth = new_depth;
        buffer.assign(depth, 0);
        read_ptr = write_ptr = 0;
        count = 0;
    }

    bool empty() const { return count == 0; }
    bool full() const { return count >= depth; }
};

#endif // SYSTOLIC_FIFO_H
