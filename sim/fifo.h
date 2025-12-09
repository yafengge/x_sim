#ifndef SYSTOLIC_FIFO_H
#define SYSTOLIC_FIFO_H

#include "systolic_common.h"
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

    bool empty() const { return count == 0; }
    bool full() const { return count >= depth; }
};

#endif // SYSTOLIC_FIFO_H
