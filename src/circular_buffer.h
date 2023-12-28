#pragma once

#include <array>
#include <atomic>

#include "spinlock.h"

template <typename T, size_t sz_pow_2>
class circular_buffer 
{
	static constexpr size_t capacity = 1 << sz_pow_2;

	std::array<T, capacity> queue;
	uint32_t read_idx;
	uint32_t write_idx;
    std::atomic<uint32_t> size{0};
	spinlock guard;
public:
	circular_buffer() : read_idx(0), write_idx(0) {}

    bool is_empty_task_queue() const
    {
        return (size.load() == 0);
    }

    bool is_full_task_queue() const 
    {
        return (size.load() == capacity);
    }

    bool try_pop_task_queue(T &item) 
    {
        guard.lock();
        if (is_empty_task_queue()) {
            guard.unlock();
            return false;
        }
        item = queue[read_idx];
        std::atomic_fetch_add(&size, -1);
        read_idx = (read_idx + 1) & (capacity - 1);
        guard.unlock();
        return true;
    }

    bool try_push_task_queue(const T task) {
        guard.lock();
        if (is_full_task_queue()) {
            guard.unlock();
            return false;
        }
        queue[write_idx] = task;
        std::atomic_fetch_add(&size, 1);
        write_idx = (write_idx + 1) & (capacity- 1);
        guard.unlock();
        return true;
    }
};

