#pragma once

#include <map>

namespace immutable::internals
{
	class MemoryBlock;

	// Immutable object allocator memory page.
	class MemoryPage
	{
	public:
		// Initialization of fields.
		MemoryPage(void* startAddress, std::size_t totalSize, std::size_t fillOffset);

		// The base address of the page in memory (just the page identifier - the memory is virtual).
		void* StartAddress;

		// Page size for the case of allocating several system pages in a row for large objects.
		std::size_t TotalSize;

		// Page padding offset.
		std::size_t FillOffset;

		// The state of memory blocks on the current page.
		std::map<void*, MemoryBlock*> MemoryBlocks;
	};
};
