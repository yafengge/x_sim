#ifndef CLOCK_H
#define CLOCK_H

#include <vector>
#include <functional>
#include <algorithm>
#include <cstdint>
#include "systolic_common.h"

class Clock {
public:
    using Listener = std::function<void()>;

    Clock() : cycle_count(0), next_id(1) {}

    // Add a listener with optional priority (lower runs earlier). Returns an id handle.
    std::size_t add_listener(const Listener &l, int priority = 0) {
        ListenerEntry e;
        e.id = next_id++;
        e.priority = priority;
        e.func = l;
        listeners.push_back(e);
        // keep listeners sorted by priority (ascending)
        std::sort(listeners.begin(), listeners.end(), [](const ListenerEntry &a, const ListenerEntry &b){
            return a.priority < b.priority;
        });
        return e.id;
    }

    // Remove a listener by id
    void remove_listener(std::size_t id) {
        listeners.erase(std::remove_if(listeners.begin(), listeners.end(), [id](const ListenerEntry &e){
            return e.id == id;
        }), listeners.end());
    }

    void tick() {
        cycle_count++;
        // Call in priority order
        for (auto &e : listeners) {
            if (e.func) e.func();
        }
    }

    Cycle now() const { return cycle_count; }

private:
    struct ListenerEntry { std::size_t id; int priority; Listener func; };
    Cycle cycle_count;
    std::size_t next_id;
    std::vector<ListenerEntry> listeners;
};

#endif // CLOCK_H
