#include "..\..\headers\internals\memory_block.h"

namespace immutable::internals
{
	MemoryBlock::MemoryBlock
	(
		void* startAddress,
		size_t totalSize,
		MemoryPage* ownerPage
	)
	{
		StartAddress = startAddress;
		TotalSize = totalSize;
		OwnerPage = ownerPage;
		IsInitialized = false;
	};
};