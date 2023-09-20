#pragma once

#ifdef __unix__
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <iostream>

#include "..\memory_page.h"

namespace immutable::internals::protectors
{
	// Wrapper around the operating system API memory management methods.
	class MemoryProtectorUnix
	{
	public:
		// Getting the memory page size depending on the platform.
		static std::size_t GetMemoryPageSize();

		// Retrieves a non-writable memory page from the system.
		static MemoryPage* CatchPage(size_t pageSize);

		// Releases a page of memory into the system.
		static void FreePage(MemoryPage* page);

		// Closes the memory page for recording.
		static void LockPage(MemoryPage* page);

		// Opens the memory page for recording.
		static void UnlockPage(MemoryPage* page);
	};
};
#endif