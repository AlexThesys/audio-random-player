#pragma once

#include <condition_variable>
#include <mutex>

class semaphore
{
	std::mutex m;
	std::condition_variable cv;
	volatile int ready = 0;
public:
	void wait() 
	{
		std::unique_lock<std::mutex> lock(m);
		//if (!this->ready) {
			cv.wait(lock, [this]() { return this->ready; });
		//}
		this->ready = 0;
	}
	void signal()
	{
		{
			std::unique_lock<std::mutex> lock(m);
			ready = 1;
		}
		cv.notify_one();
	}
};