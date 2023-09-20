#ifdef __unix__

#include "..\..\..\headers\internals\protectors\memory_protector_unix.h"

namespace immutable::internals::protectors
{
	std::size_t MemoryProtectorUnix::GetMemoryPageSize()
	{
		return sysconf(_SC_PAGE_SIZE);
	};

	MemoryPage* MemoryProtectorUnix::CatchPage(size_t pageSize)
	{
		auto result = mmap(nullptr, pageSize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (result != -1) return new MemoryPage(result, pageSize, 0);
		auto message = strerror(errno);
		throw std::runtime_error(message);
	};

	void MemoryProtectorUnix::FreePage(MemoryPage* page)
	{
		auto success = munmap(page.StartAddress, page.TotalSize);
		if (success == 0) return;
		auto message = strerror(errno);
		throw std::runtime_error(message);
	};

	void MemoryProtectorUnix::LockPage(MemoryPage* page)
	{
		auto success = mprotect(page.StartAddress, page.TotalSize, PROT_READ);
		if (success == 0) return;
		auto message = strerror(errno);
		throw std::runtime_error(message);
	};

	void MemoryProtectorUnix::UnlockPage(MemoryPage* page)
	{
		auto success = mprotect(page.StartAddress, page.TotalSize, PROT_READ | PROT_WRITE);
		if (success == 0) return;
		auto message = strerror(errno);
		throw std::runtime_error(message);
	};
};
#endif