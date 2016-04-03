#pragma once
#include <mutex>
#define GIMME SourceInfo {__FILE__, __func__, __LINE__}

#ifdef _WIN32
#define MEM_CALL __cdecl
#else
// TODO
#define MEM_CALL
#endif

namespace memory {
	// Wraps a pointer to always be 64-bit
	// for alignment purposes
	template<typename T>
	struct ptr64_t {
	private:
		uint64_t ptr;

	public:
		operator T*() {
			return (T*) ptr;
		}

		T* operator()() {
			return ptr;
		}

		ptr64_t<T> operator=(T* t) {
			ptr = (uint64_t) t;
			return *this;
		}
	};

	inline char* Align(char* address, std::size_t alignment) {
		return (char*) (((std::size_t) address + alignment - 1) & ~(alignment - 1));
	}

#if 0
	struct SourceInfo {
		// File name
		// Function name
		// Line number
	};

	template <
		class AllocationPolicy,
		class ThreadPolicy,
		class BoundsCheckingPolicy,
		class MemoryTrackingPolicy,
		class MemoryTaggingPolicy
	>
	class MemoryArena
	{
	public:
		template <class AreaPolicy>
		explicit MemoryArena(AreaPolicy* area)
			: m_allocator(area->GetStart(), area->GetEnd())
		{
		}

		void* Allocate(std::size_t size, std::size_t alignment, SourceInfo& sourceInfo)
		{
			m_threadGuard.Enter();

			const std::size_t originalSize = size;
			const std::size_t newSize = size + BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;

			char* plainMemory = static_cast<char*>(m_allocator.Allocate(newSize, alignment, BoundsCheckingPolicy::SIZE_FRONT));

			m_boundsChecker.GuardFront(plainMemory);
			m_memoryTagger.TagAllocation(plainMemory + BoundsCheckingPolicy::SIZE_FRONT, originalSize);
			m_boundsChecker.GuardBack(plainMemory + BoundsCheckingPolicy::SIZE_FRONT + originalSize);

			m_memoryTracker.OnAllocation(plainMemory, newSize, alignment, sourceInfo);

			m_threadGuard.Leave();

			return (plainMemory + BoundsCheckingPolicy::SIZE_FRONT);
		}

		void Free(void* ptr)
		{
			m_threadGuard.Enter();

			char* originalMemory = static_cast<char*>(ptr) - BoundsCheckingPolicy::SIZE_FRONT;
			const std::size_t allocationSize = m_allocator.GetAllocationSize(originalMemory);

			m_boundsChecker.CheckFront(originalMemory);
			m_boundsChecker.CheckBack(originalMemory + allocationSize - BoundsCheckingPolicy::SIZE_BACK);

			m_memoryTracker.OnDeallocation(originalMemory);

			m_memoryTagger.TagDeallocation(originalMemory, allocationSize);

			m_allocator.Free(originalMemory);

			m_threadGuard.Leave();
		}

	private:
		AllocationPolicy m_allocator;
		ThreadPolicy m_threadGuard;
		BoundsCheckingPolicy m_boundsChecker;
		MemoryTrackingPolicy m_memoryTracker;
		MemoryTaggingPolicy m_memoryTagger;
	};

	class NoBoundsChecking
	{
	public:
		static const std::size_t SIZE_FRONT = 0;
		static const std::size_t SIZE_BACK = 0;

		inline void GuardFront(void*) const {}
		inline void GuardBack(void*) const {}

		inline void CheckFront(const void*) const {}
		inline void CheckBack(const void*) const {}
	};

	class NoMemoryTracking
	{
	public:
		inline void OnAllocation(void*, std::size_t, std::size_t, const SourceInfo&) const {}
		inline void OnDeallocation(void*) const {}
	};

	class NoMemoryTagging
	{
	public:
		inline void TagAllocation(void*, std::size_t) const {}
		inline void TagDeallocation(void*, std::size_t) const {}
	};

	class SingleThreadPolicy
	{
	public:
		inline void Enter(void) const {}
		inline void Leave(void) const {}
	};

	class MultiThreadPolicy
	{
	public:
		inline void Enter(void)
		{
			m_primitive.lock();
		}

		inline void Leave(void)
		{
			m_primitive.unlock();
		}

	private:
		std::mutex m_primitive;
	};

	template <class Arena>
	class RecordingArena
	{
	public:
		void* Allocate(std::size_t size, std::size_t alignment, const SourceInfo& sourceInfo)
		{
			// send info via TCP/IP...
			return m_arena.Allocate(size, alignment, sourceInfo);
		}

		void Free(void* ptr)
		{
			// send info via TCP/IP...
			m_arena.Free(ptr);
		}

	private:
		Arena m_arena;
	};

	// Allocators
	class LinearAllocator
	{
	public:
		//explicit LinearAllocator(std::size_t size);
		LinearAllocator(void* start, void* end) :
			m_start(static_cast<char*>(start)),
			m_end(static_cast<char*>(end)),
			m_current(m_start) {}

		void* Allocate(std::size_t size, std::size_t alignment, std::size_t offset) {
			// offset pointer first, align it, and offset it back
			m_current = Align(m_current + offset, alignment) - offset;

			void* userPtr = m_current;
			m_current += size;

			if (m_current >= m_end)
			{
				// out of memory
				return nullptr;
			}

			return userPtr;
		}

		inline void Free(void* ptr);

		inline void Reset(void);

	private:
		char* m_start;
		char* m_end;
		char* m_current;
	};

	//typedef MemoryArena<PoolAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> ST_PoolStackArena;
	typedef MemoryArena<LinearAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> SimpleArena;
	//typedef MemoryArena<StackBasedAllocator, MultiThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> MT_StackBasedArena;
#endif
}
