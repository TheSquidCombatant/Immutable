#include "..\..\headers\internals\memory_page.h"

namespace immutable::internals
{
	MemoryPage::MemoryPage
	(
		void* startAddress,
		std::size_t totalSize,
		std::size_t fillOffset
	)
	{
		StartAddress = startAddress;
		TotalSize = totalSize;
		FillOffset = fillOffset;
		MemoryBlocks = std::map<void*, MemoryBlock*>::map();
	};
};