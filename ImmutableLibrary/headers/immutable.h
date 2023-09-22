#pragma once

#include <iostream>
#include <list>
#include <mutex>
#include <memory>
#include <map>

#ifdef __unix__
#include "internals\protectors\memory_protector_unix.h"
typedef immutable::internals::protectors::MemoryProtectorUnix MemoryProtector;
#elif defined(_WIN32)
#include "internals\protectors\memory_protector_windows.h"
typedef immutable::internals::protectors::MemoryProtectorWindows MemoryProtector;
#else
// If you want to expand functionality for the new platform, you can add your memory proitector declaration here!
#error You must define at least one of the tokens __unix__ or _WIN32.
#endif

#include "internals\memory_block.h"
#include "internals\memory_page.h"

using namespace immutable::internals;

namespace immutable
{
	// Storage for memory allocation internal data.
	class ImmutableData
	{
	private:
		// Let him have access to storage for memory allocation data.
		template<class T> friend class ImmutableAllocator;

		// An object for synchronizing work with an allocator (someday the synchronization will be rewritten).
		static inline std::mutex Mutex;

		// Operating system memory page size.
		static inline std::size_t SystemPageSize;

		// Allocator-managed memory pages, in order of increasing free space on the page.
		static inline std::list<MemoryPage*> MemoryPages;

		// Allocator-managed memory blocks with no ordering.
		static inline std::map<void*, MemoryBlock*> MemoryBlocks;
	};

	// A wrapper for pinning an immutable object to the local scope.
	template<class T> class ImmutableGuard
	{
	public:
		// A wrapped immutable object.
		T* WrappedObject;

		// A constructor with an wrapped object already created by an immutable allocator.
		ImmutableGuard(T* wrappedObject);

		// A constructor that itself creates a wrapped object through an immutable allocator.
		template<class... Args> ImmutableGuard(Args&&... args);

		// It will properly clean up after the wrapped object when the wrapper is destroyed.
		~ImmutableGuard();
	};

	// Memory allocator for immutable objects.
	template<class T> class ImmutableAllocator
	{
	public:
		// For some reason, this typedef is required.
		typedef T value_type;

		// Filling static fields in the case of a default constructor.
		ImmutableAllocator();

		// Filling static fields in the case of a copy constructor.
		template<class U> ImmutableAllocator(const ImmutableAllocator<U>& other);

		// Interface method for allocating memory for an allocator trait from std.
		T* allocate(std::size_t count_objects);

		// Interface method for initializing an object for an allocator trait from std.
		template<class U, class... Args> void construct(U* p, Args&&... args);

		// Interface method for deinitializing an object for an allocator trait from std.
		template<class U> void destroy(U* p);

		// Interface method for freeing memory for the allocator trait from std.
		void deallocate(T* ptr, std::size_t count_objects);

		// For some reason, an equality operator is required.
		template<class U> bool operator==(const ImmutableAllocator<U>& other);

		// For some reason, an inequality operator is required.
		template<class U> bool operator!=(const ImmutableAllocator<U>& other);

	private:
		// Takes a free block of memory from an existing page (or creates a new one for this purpose).
		static MemoryBlock* CatchBlocksAndReturnFirst(size_t blockSize, size_t blockCount);

		// Releases the memory blocks on the page and the page itself if it is empty.
		static void FreeBlock(void* startAddress, size_t blockSize, size_t blockCount);

		// Let him have access to internal methods just in case.
		friend class ImmutableGuard<T>;

		// Looks for a memory page with enough free space. If success returns page position in cache else return end position.
		static std::list<MemoryPage*>::iterator FindMemoryPagePositionWithEnoughSpace(size_t minFreeSpace);

		// Searches for a memory blocks sequence by first block starting address. If success returns first block else throws an exception.
		static MemoryBlock* FindMemoryBlocksAndReturnFirst(void* startAddress, size_t blockSize, size_t blockCount);

		// Inserts a page into the list of managed pages in order of increasing free memory.
		static void InsertMemoryPageInCache(MemoryPage* page);
	};
};

#include "..\source\immutable_allocator.h"
#include "..\source\immutable_guard.h"