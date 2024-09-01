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
	volatile LONG has_new_data;
public:
	tripple_buffer() : has_new_data(1)
	{
		front_buffer_ptr = &data[2];
		back_buffer_ptr = &data[0];
		middle_buffer_ptr = &data[1];
	}
	T* get_data(size_t id) { return &data[id]; }
	T* get_back_buffer() { return back_buffer_ptr; }
	void publish() 
	{
		back_buffer_ptr = (T*)InterlockedExchange64((volatile LONG64*)&middle_buffer_ptr, reinterpret_cast<LONG64>(back_buffer_ptr));
		InterlockedExchange(&has_new_data, 1); // InterlockedExchange is used for serialization
	}
	T* const consume()
	{
		if (InterlockedExchange(&has_new_data, 0)) {
			front_buffer_ptr = (T*)InterlockedExchange64((volatile LONG64*)&middle_buffer_ptr, reinterpret_cast<LONG64>(front_buffer_ptr));
		}
		return front_buffer_ptr;
	}
	T* const try_consume()
	{
		if (InterlockedExchange(&has_new_data, 0)) {
			front_buffer_ptr = (T*)InterlockedExchange64((volatile LONG64*)&middle_buffer_ptr, reinterpret_cast<LONG64>(front_buffer_ptr));
			return front_buffer_ptr;
		}
		return nullptr;
	}
};

class tripple_indices {
	std::array<volatile uint64_t, 3> data;
	volatile LONG has_new_data;

public:
	tripple_indices() : has_new_data(1)
	{
		data[0] = 0; // back buffer
		data[1] = 1; // shared buffer
		data[2] = 2; // front buffer
	}
	uint64_t get_back_buffer() { return data[0]; }
	void publish()
	{
		data[0] = (uint64_t)InterlockedExchange64((volatile LONG64*)(&data[1]), static_cast<LONG64>(data[0]));
		InterlockedExchange(&has_new_data, 1); // InterlockedExchange is used for serialization
	}
	uint64_t const consume()
	{
		if (InterlockedExchange(&has_new_data, 0)) {
			data[2] = (uint64_t)InterlockedExchange64((volatile LONG64*)(&data[1]), static_cast<LONG64>(data[2]));
		}
		return data[2];
	}
};