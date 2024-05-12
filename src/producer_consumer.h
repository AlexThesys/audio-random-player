#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#include "semaphore.h"

// consumer thread can wait for data, but producer can't be blocked
template <typename data_buffer>
class producer_consumer {
    public:
        static constexpr uint64_t guard = uint64_t(-1);
    private:
        data_buffer _front_buf;
        data_buffer _back_buf;
        data_buffer *_front_buf_ptr;
        data_buffer *_back_buf_ptr;
        volatile data_buffer *_shared_buf_ptr;
        data_buffer *_save_buf_ptr;
        volatile uint32_t _new_data_ready;
        //volatile uint32_t _stop_consumption;    // if consumer is going to run a while loop, this is the exit condition
        semaphore _sem;	// binary semaphore

        void atomic_store(volatile uint64_t& ptr, uint64_t val) { ptr = val; }
        void atomic_store(volatile uint32_t& ptr, uint32_t val) { ptr = val; }

    public:
        producer_consumer () : _front_buf_ptr(&_front_buf), _back_buf_ptr(&_back_buf), _shared_buf_ptr(_back_buf_ptr), 
                                _save_buf_ptr(nullptr), _new_data_ready(0)//, _stop_consumption(0) 
        {}

        ~producer_consumer () 
        {
            deinit();
        }
        void deinit()
        {
            //atomic_store((volatile uint32_t&)_stop_consumption, 1u);
            atomic_store((volatile uint32_t&)_new_data_ready, 1u);
            _sem.signal();
        }
        // called from the producer thread
        data_buffer &begin_producing() 
        {
            _save_buf_ptr	= (data_buffer*)(InterlockedExchange64(((volatile LONG64*)&_shared_buf_ptr), guard));
            return			*_save_buf_ptr;
        }
        void end_producing() 
        {
            atomic_store((volatile uint64_t&)_shared_buf_ptr, (uint64_t)_save_buf_ptr);
            atomic_store((volatile uint32_t&)_new_data_ready, 1u);
            _sem.signal();
        }
        // called from the consumer thread
        data_buffer &begin_consuming() 
        {
            wait_for_data	();
            swap			(_front_buf_ptr, _back_buf_ptr);	// doesn't need to be atomic
            return			*_front_buf_ptr;
        }
        // no need to clear - in our case the data just going to be rewritten
        //void end_consuming() 
        //{
        //    _front_buf_ptr->clear();
        //}

        typedef void(*init_func)(void*, void*);
        void init_data(init_func fp)
        {
            fp(&_front_buf, &_back_buf);
        }

private:
        void wait_for_data() 
        {    // atomic_cmpxchg(dst*, xchg, cmp)->old
            while (!InterlockedExchange((volatile uint32_t*)&_new_data_ready, 0u) || (guard == InterlockedCompareExchange64((volatile int64_t*)&_shared_buf_ptr, (int64_t)_front_buf_ptr, (int64_t)_back_buf_ptr))) {
                _sem.wait();	// doesn't wait, if the object is already signaled
            }
        }

        static void swap(data_buffer* a, data_buffer* b)
        {
            data_buffer* temp = b;
            b = a;
            a = temp;
        }
};