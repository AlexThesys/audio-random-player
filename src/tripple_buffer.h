#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>

#include "constants.h"

template <typename T>
class tripple_buffer {
	std::array<T, 3> data;
	struct {
		T *front_buffer_ptr;
		T *back_buffer_ptr;
		volatile T *middle_buffer_ptr;
	};
	volatile int has_new_data;
public:
	tripple_buffer() : has_new_data(1)
	{
		front_buffer_ptr = data[1];
		back_buffer_ptr = data[0];
		middle_buffer_ptr = data[2];
	}
	T* get_data(size_t id) { return data[id]; }
	T* get_back_buffer() { return back_buffer_ptr; }
	void produce() 
	{
		back_buffer_ptr = (T*)InterlockedExchange64((volatile LONG64*)&middle_buffer_ptr, reinterpret_cast<LONG64>(back_buffer_ptr));
		has_new_data = 1;
	}
	T* const consume()
	{
		if (InterlockedCompareExchange(&has_new_data, 0, 1)) {
			front_buffer_ptr = (T*)InterlockedExchange64((volatile LONG64*)&middle_buffer_ptr, reinterpret_cast<LONG64>(front_buffer_ptr));
		}
	}
};