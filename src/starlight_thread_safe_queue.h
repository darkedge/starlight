#pragma once
#include <queue>
#include <mutex>

namespace util {
	// Single producer, single consumer thread safe queue.
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
}
