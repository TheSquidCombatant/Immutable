#ifdef _WIN32

#include "..\..\..\headers\internals\protectors\memory_protector_windows.h"

namespace immutable::internals::protectors
{
	std::size_t MemoryProtectorWindows::GetMemoryPageSize()
	{
		SYSTEM_INFO siSysInfo;
		GetSystemInfo(&siSysInfo);
		return siSysInfo.dwPageSize;
	};

	MemoryPage* MemoryProtectorWindows::CatchPage(size_t pageSize)
	{
		auto result = VirtualAlloc(nullptr, pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READONLY);
		if (result != nullptr) return new MemoryPage(result, pageSize, 0);
		auto message = std::system_category().message(::GetLastError());
		throw std::runtime_error(message);
	};

	void MemoryProtectorWindows::FreePage(MemoryPage* page)
	{
		auto success = VirtualFree(page->StartAddress, 0, MEM_RELEASE);
		if (success != 0) return;
		auto message = std::system_category().message(::GetLastError());
		throw std::runtime_error(message);
	};

	void MemoryProtectorWindows::LockPage(MemoryPage* page)
	{
		DWORD old;
		auto success = VirtualProtect(page->StartAddress, page->TotalSize, PAGE_READONLY, &old);
		if (success != 0) return;
		auto message = std::system_category().message(::GetLastError());
		throw std::runtime_error(message);
	};

	void MemoryProtectorWindows::UnlockPage(MemoryPage* page)
	{
		DWORD old;
		auto success = VirtualProtect(page->StartAddress, page->TotalSize, PAGE_READWRITE, &old);
		if (success != 0) return;
		auto message = std::system_category().message(::GetLastError());
		throw std::runtime_error(message);
	};
};
#endif