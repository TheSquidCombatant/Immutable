#pragma once

#include "..\headers\immutable.h"

namespace immutable
{
	template<class T> T* ImmutableAllocator<T>::allocate(size_t count_objects)
	{
		// a stub for allocating memory for std container's internals
		if (typeid(T) == typeid(_Container_proxy))
			return (T*)malloc(sizeof(T) * count_objects);
		// regular memory allocation by allocator
		const lock_guard<mutex> guard(ImmutableData::Mutex);
		auto firstCatchedBlock = CatchBlocksAndReturnFirst(sizeof(T), count_objects);
		return (T*)(firstCatchedBlock->StartAddress);
	};

	template<class T> template<class U, class... Args> void ImmutableAllocator<T>::construct(U* p, Args&&... args)
	{
		const lock_guard<mutex> guard(ImmutableData::Mutex);
		auto firstFoundBlock = FindMemoryBlocksAndReturnFirst(p, sizeof(U), 1);
		constexpr auto alreadyInitialized = "Memory block is already initialized.";
		if (firstFoundBlock->IsInitialized)
			throw runtime_error(alreadyInitialized);
		MemoryProtector::UnlockPage(firstFoundBlock->OwnerPage);
		construct_at<U>(p, forward<Args>(args)...);
		MemoryProtector::LockPage(firstFoundBlock->OwnerPage);
		firstFoundBlock->IsInitialized = true;
	};

	template<class T> template<class U> void ImmutableAllocator<T>::destroy(U* p)
	{
		const lock_guard<mutex> guard(ImmutableData::Mutex);
		auto firstFoundBlock = FindMemoryBlocksAndReturnFirst(p, sizeof(U), 1);
		constexpr auto notInitialized = "Memory block is not deinitialized.";
		if (!firstFoundBlock->IsInitialized)
			throw runtime_error(notInitialized);
		MemoryProtector::UnlockPage(firstFoundBlock->OwnerPage);
		destroy_at<U>(p);
		MemoryProtector::LockPage(firstFoundBlock->OwnerPage);
		firstFoundBlock->IsInitialized = false;
	}

	template<class T> void ImmutableAllocator<T>::deallocate(T* ptr, size_t count_objects)
	{
		// a stub for deallocating memory for std container's internals
		if (typeid(T) == typeid(_Container_proxy))
			return free(ptr);
		// regular memory deallocation by allocator
		const lock_guard<mutex> guard(ImmutableData::Mutex);
		auto firstFoundBlock = FindMemoryBlocksAndReturnFirst(ptr, sizeof(T), count_objects);
		FreeBlocks(ptr, sizeof(T), count_objects);
	};

	template<class T> MemoryBlock* ImmutableAllocator<T>::CatchBlocksAndReturnFirst(size_t blockSize, size_t blockCount)
	{
		auto totalBlockSize = blockSize * blockCount;
		auto targetPagePosition = FindMemoryPagePositionWithEnoughSpace(totalBlockSize);
		MemoryPage* targetPage = nullptr;
		MemoryBlock* firstCatchedBlock = nullptr;

		if (targetPagePosition == ImmutableData::MemoryPages.end())
		{
			auto systemPageSize = ImmutableData::SystemPageSize;
			auto pageSize = ((totalBlockSize % systemPageSize == 0) ? totalBlockSize : (((totalBlockSize / systemPageSize) + 1) * systemPageSize));
			targetPage = MemoryProtector::CatchPage(pageSize);
		}
		else
		{
			targetPage = (*targetPagePosition);
			ImmutableData::MemoryPages.erase(targetPagePosition);
		}

		for (size_t i = 0; i < blockCount; ++i)
		{
			auto block = new MemoryBlock((char*)targetPage->StartAddress + targetPage->FillOffset, blockSize, targetPage);
			targetPage->FillOffset += blockSize;
			if (firstCatchedBlock == nullptr)
				firstCatchedBlock = block;
			ImmutableData::MemoryBlocks[block->StartAddress] = block;
		}

		targetPage->BlocksCount += blockCount;
		InsertMemoryPageInCache(targetPage);
		return firstCatchedBlock;
	};

	template<class T> void ImmutableAllocator<T>::FreeBlocks(void* startAddress, size_t blockSize, size_t blockCount)
	{
		constexpr auto notDeinitialized = "Specified block is not deinitializes.";
		MemoryPage* page = nullptr;

		for (size_t i = 0; i < blockCount; ++i)
		{
			auto search = ImmutableData::MemoryBlocks.find(startAddress);
			if (search->second->IsInitialized)
				throw runtime_error(notDeinitialized);
			if (page == nullptr)
				page = search->second->OwnerPage;
			delete search->second;
			ImmutableData::MemoryBlocks.erase(startAddress);
			startAddress = (char*)startAddress + blockSize;
		}

		page->BlocksCount -= blockCount;

		if (page->BlocksCount == 0)
		{
			MemoryProtector::FreePage(page);
			ImmutableData::MemoryPages.remove(page);
			delete page;
		}
	};

	template<class T> list<MemoryPage*>::iterator ImmutableAllocator<T>::FindMemoryPagePositionWithEnoughSpace(size_t minFreeSpace)
	{
		if (ImmutableData::MemoryPages.empty())
			return ImmutableData::MemoryPages.end();
		if (ImmutableData::MemoryPages.back()->TotalSize - ImmutableData::MemoryPages.back()->FillOffset < minFreeSpace)
			return ImmutableData::MemoryPages.end();
		list<MemoryPage*>::iterator it;
		for (it = ImmutableData::MemoryPages.begin(); it != ImmutableData::MemoryPages.end(); ++it)
			if ((*it)->TotalSize - (*it)->FillOffset >= minFreeSpace)
				break;
		return it;
	};

	template<class T> MemoryBlock* ImmutableAllocator<T>::FindMemoryBlocksAndReturnFirst(void* startAddress, size_t blockSize, size_t blockCount)
	{
		constexpr auto corruptedBlockStatus = "Memory block status is corrupted.";
		constexpr auto corruptedPageStatus = "Memory page status is corrupted.";

		auto search = ImmutableData::MemoryBlocks.find(startAddress);
		if (search == ImmutableData::MemoryBlocks.end())
			throw runtime_error(corruptedPageStatus);
		MemoryBlock* firstFoundBlock = search->second;
		size_t foundBlockCount = 0;

		while (search != ImmutableData::MemoryBlocks.end())
		{
			if (search->second->TotalSize != blockSize)
				throw runtime_error(corruptedBlockStatus);
			if (++foundBlockCount == blockCount)
				break;
			startAddress = (char*)startAddress + blockSize;
			search = ImmutableData::MemoryBlocks.find(startAddress);
		}

		if (foundBlockCount == blockCount)
			return firstFoundBlock;
		throw runtime_error(corruptedPageStatus);
	};

	template<class T> void ImmutableAllocator<T>::InsertMemoryPageInCache(MemoryPage* page)
	{
		auto insertPagePosition = FindMemoryPagePositionWithEnoughSpace(page->TotalSize - page->FillOffset);
		if (insertPagePosition == ImmutableData::MemoryPages.end())
			ImmutableData::MemoryPages.push_back(page);
		else
			ImmutableData::MemoryPages.insert(insertPagePosition, page);
	};
};