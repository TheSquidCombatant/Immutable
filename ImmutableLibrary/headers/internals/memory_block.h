#pragma once

#include <cstddef>

#include "memory_page.h"

using namespace std;

namespace immutable::internals
{
	// Information about allocated memory block.
	class MemoryBlock
	{
	public:
		// Initialization of fields.
		MemoryBlock(void* startAddress, size_t totalSize, MemoryPage* ownerPage);

		// Address of the block in memory.
		void* StartAddress;

		// The size of a memory block.
		size_t TotalSize;

		// A pointer to the memory page where the memory block is located.
		MemoryPage* OwnerPage;

		// A sign that the memory block is initialized with value.
		bool IsInitialized;
	};
};