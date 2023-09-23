#pragma once

#include <map>

using namespace std;

namespace immutable::internals
{
	// Information about allocated memory page.
	class MemoryPage
	{
	public:
		// Initialization of fields.
		MemoryPage(void* startAddress, size_t totalSize);

		// The base address of the page in memory (just the page identifier - the memory is virtual).
		void* StartAddress;

		// Page size for the case of allocating several system pages in a row for large objects.
		size_t TotalSize;

		// Page padding offset.
		size_t FillOffset;

		// Count of associated memory blocks.
		size_t BlocksCount;
	};
};
