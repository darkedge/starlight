#pragma once
#include <queue>
#include <mutex>

namespace util {
	// Single producer, single consumer thread safe queue.
	// Does not block.
	template<typename T>
	class ThreadSafeQueue {
	public:
		void Enqueue(T t)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_queue.push(t);
		}

		bool Dequeue(T* t)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_queue.empty()) {
				return false;
			} else {
				*t = m_queue.front();
				m_queue.pop();
				return true;
			}
		}
	private:
		std::queue<T> m_queue;
		mutable std::mutex m_mutex;
	};

	template<typename T>
	class BlockingQueue {
	public:
		void Enqueue(T* t) {
			std::unique_lock<std::mutex> lock(mutex);
			queue.push(t);
			lock.unlock();
			cv.notify_one();
		}

		void Dequeue(T* t) {
			std::unique_lock<std::mutex> lk(mutex);
			cv.wait(lk); // can use wait_for to timeout, return false
			//std::unique_lock<std::mutex> lock(mutex); // Needed?
			*t = queue.front();
			queue.pop();
		}

	private:
		std::condition_variable cv;
		mutable std::mutex mutex;
		//mutable std::mutex cv_m;
		std::queue<T> queue;
	};
}
