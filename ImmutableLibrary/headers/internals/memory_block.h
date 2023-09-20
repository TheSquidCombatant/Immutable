#pragma once

#include <cstddef>

namespace immutable::internals
{
	class MemoryPage;

	// Immutable object allocator memory block.
	class MemoryBlock
	{
	public:
		// Initialization of fields.
		MemoryBlock(void* startAddress, std::size_t totalSize, MemoryPage* ownerPage);

		// Address of the block in memory.
		void* StartAddress;

		// The size of a memory block.
		std::size_t TotalSize;

		// A pointer to the memory page where the memory block is located.
		MemoryPage* OwnerPage;

		// A sign that the memory block has been freed.
		bool IsReleased;
	};
};