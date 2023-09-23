#include "..\..\headers\internals\memory_page.h"

namespace immutable::internals
{
	MemoryPage::MemoryPage
	(
		void* startAddress,
		size_t totalSize
	)
	{
		StartAddress = startAddress;
		TotalSize = totalSize;
		FillOffset = 0;
		BlocksCount = 0;
	};
};