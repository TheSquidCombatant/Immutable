#pragma once

#include "..\headers\immutable.h"

namespace immutable
{
	template<class T> ImmutableAllocator<T>::ImmutableAllocator()
	{
		if (ImmutableData::SystemPageSize == 0) ImmutableData::SystemPageSize = MemoryProtector::GetMemoryPageSize();
	};

	template<class T> template<class U> ImmutableAllocator<T>::ImmutableAllocator(const ImmutableAllocator<U>& other)
	{
		if (ImmutableData::SystemPageSize == 0) ImmutableData::SystemPageSize = MemoryProtector::GetMemoryPageSize();
	};

	template<class T> T* ImmutableAllocator<T>::allocate(std::size_t count_objects)
	{
		if (typeid(T) == typeid(std::_Container_proxy)) return (T*)malloc(sizeof(T) * count_objects);
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto firstCatchedBlock = CatchBlocksAndReturnFirst(sizeof(T), count_objects);
		return (T*)(firstCatchedBlock->StartAddress);
	};

	template<class T> template<class U, class... Args> void ImmutableAllocator<T>::construct(U* p, Args&&... args)
	{
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto firstFoundBlock = FindMemoryBlocksAndReturnFirst(p, sizeof(T), 1);
		const auto alreadyInitialized = "Memory block is already initialized.";
		if (firstFoundBlock->IsInitialized) throw std::runtime_error(alreadyInitialized);
		MemoryProtector::UnlockPage(firstFoundBlock->OwnerPage);
		std::construct_at<U>(p, std::forward<Args>(args)...);
		MemoryProtector::LockPage(firstFoundBlock->OwnerPage);
		firstFoundBlock->IsInitialized = true;
	};

	template<class T> template<class U> void ImmutableAllocator<T>::destroy(U* p)
	{
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto firstFoundBlock = FindMemoryBlocksAndReturnFirst(p, sizeof(T), 1);
		const auto notInitialized = "Memory block is not deinitialized.";
		if (!firstFoundBlock->IsInitialized) throw std::runtime_error(notInitialized);
		MemoryProtector::UnlockPage(firstFoundBlock->OwnerPage);
		std::destroy_at<U>(p);
		MemoryProtector::LockPage(firstFoundBlock->OwnerPage);
		firstFoundBlock->IsInitialized = false;
	}

	template<class T> void ImmutableAllocator<T>::deallocate(T* ptr, std::size_t count_objects)
	{
		if (typeid(T) == typeid(std::_Container_proxy)) return free(ptr);
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto firstFoundBlock = FindMemoryBlocksAndReturnFirst(ptr, sizeof(T), count_objects);
		FreeBlock(ptr, sizeof(T), 1);
	};

	template<class T> template<class U> bool ImmutableAllocator<T>::operator==(const ImmutableAllocator<U>& other)
	{
		return (this == other);
	};

	template<class T> template<class U> bool ImmutableAllocator<T>::operator!=(const ImmutableAllocator<U>& other)
	{
		return (this != other);
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
			if (firstCatchedBlock == nullptr) firstCatchedBlock = block;
			ImmutableData::MemoryBlocks[block->StartAddress] = block;
		}

		targetPage->BlocksCount += blockCount;
		InsertMemoryPageInCache(targetPage);
		return firstCatchedBlock;
	};

	template<class T> void ImmutableAllocator<T>::FreeBlock(void* startAddress, size_t blockSize, size_t blockCount)
	{
		const auto notDeinitialized = "Specified block is not deinitializes.";
		auto search = ImmutableData::MemoryBlocks.find(startAddress);
		auto page = search->second->OwnerPage;

		for (size_t i = 1; i < blockCount; ++i)
		{
			if (search->second->IsInitialized) throw std::runtime_error(notDeinitialized);
			ImmutableData::MemoryBlocks.erase(startAddress);
			delete (*search).second;
			startAddress = (char*)startAddress + blockSize;
			search = ImmutableData::MemoryBlocks.find(startAddress);
		}

		page->BlocksCount -= blockCount;

		if (page->BlocksCount == 0)
		{
			MemoryProtector::FreePage(page);
			auto targetPagePosition = std::find(ImmutableData::MemoryPages.begin(), ImmutableData::MemoryPages.end(), page);
			ImmutableData::MemoryPages.erase(targetPagePosition);
		}
	};

	template<class T> std::list<MemoryPage*>::iterator ImmutableAllocator<T>::FindMemoryPagePositionWithEnoughSpace(size_t minFreeSpace)
	{
		if (ImmutableData::MemoryPages.size() == 0) return ImmutableData::MemoryPages.end();
		if (ImmutableData::MemoryPages.back()->TotalSize - ImmutableData::MemoryPages.back()->FillOffset < minFreeSpace) return ImmutableData::MemoryPages.end();
		std::list<MemoryPage*>::iterator it;
		for (it = ImmutableData::MemoryPages.begin(); it != ImmutableData::MemoryPages.end(); ++it)
			if ((*it)->TotalSize - (*it)->FillOffset >= minFreeSpace)
				break;
		return it;
	};

	template<class T> MemoryBlock* ImmutableAllocator<T>::FindMemoryBlocksAndReturnFirst(void* startAddress, size_t blockSize, size_t blockCount)
	{
		const auto corruptedBlockStatus = "Memory block status is corrupted.";
		const auto corruptedPageStatus = "Memory page status is corrupted.";

		auto search = ImmutableData::MemoryBlocks.find(startAddress);
		if (search == ImmutableData::MemoryBlocks.end()) throw std::runtime_error(corruptedPageStatus);
		MemoryBlock* firstFoundBlock = search->second;
		size_t foundBlockCount = 0;

		while (search != ImmutableData::MemoryBlocks.end())
		{
			if (search->second->TotalSize != blockSize) throw std::runtime_error(corruptedBlockStatus);
			if (++foundBlockCount == blockCount) break;
			startAddress = (char*)startAddress + blockSize;
			search = ImmutableData::MemoryBlocks.find(startAddress);
		}

		if (foundBlockCount == blockCount) return firstFoundBlock;
		throw std::runtime_error(corruptedPageStatus);
	};

	template<class T> void ImmutableAllocator<T>::InsertMemoryPageInCache(MemoryPage* page)
	{
		auto insertPagePosition = FindMemoryPagePositionWithEnoughSpace(page->TotalSize - page->FillOffset);
		if (insertPagePosition == ImmutableData::MemoryPages.end()) ImmutableData::MemoryPages.push_back(page);
		else ImmutableData::MemoryPages.insert(insertPagePosition, page);
	};
};