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
		auto blocks = CatchBlocks(sizeof(T), count_objects);
		return (T*)(blocks->front()->StartAddress);
	};

	template<class T> template<class U, class... Args> void ImmutableAllocator<T>::construct(U* p, Args&&... args)
	{
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto blocks = FindMemoryBlocksByStartAddress(p, sizeof(T), 1);
		MemoryProtector::UnlockPage(blocks->front()->OwnerPage);
		std::construct_at<U>(p, std::forward<Args>(args)...);
		MemoryProtector::LockPage(blocks->front()->OwnerPage);
	};

	template<class T> template<class U> void ImmutableAllocator<T>::destroy(U* p)
	{
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto blocks = FindMemoryBlocksByStartAddress(p, sizeof(T), 1);
		MemoryProtector::UnlockPage(blocks->front()->OwnerPage);
		std::destroy_at<U>(p);
		MemoryProtector::LockPage(blocks->front()->OwnerPage);
	}

	template<class T> void ImmutableAllocator<T>::deallocate(T* ptr, std::size_t count_objects)
	{
		if (typeid(T) == typeid(std::_Container_proxy)) return free(ptr);
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto blocks = FindMemoryBlocksByStartAddress(ptr, sizeof(T), count_objects);
		for (auto block : (*blocks)) FreeBlock(block);
	};

	template<class T> template<class U> bool ImmutableAllocator<T>::operator==(const ImmutableAllocator<U>& other)
	{
		return (this == other);
	};

	template<class T> template<class U> bool ImmutableAllocator<T>::operator!=(const ImmutableAllocator<U>& other)
	{
		return (this != other);
	};

	template<class T> std::list<MemoryBlock*>* ImmutableAllocator<T>::CatchBlocks(size_t blockSize, size_t blockCount)
	{
		auto totalBlockSize = blockSize * blockCount;
		auto targetPagePosition = FindMemoryPagePositionWithEnoughSpace(totalBlockSize);
		MemoryPage* targetPage = nullptr;
		auto resultBlocks = new std::list<MemoryBlock*>();

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
			targetPage->MemoryBlocks.push_back(block);
			resultBlocks->push_back(block);
		}

		InsertMemoryPageInCache(targetPage);
		return resultBlocks;
	};

	template<class T> void ImmutableAllocator<T>::FreeBlock(MemoryBlock* block)
	{
		// if the memory page has already been freed - an error
		const auto missingPage = "No corresponding memory page found.";
		auto targetPagePosition = std::find(ImmutableData::MemoryPages.begin(), ImmutableData::MemoryPages.end(), block->OwnerPage);
		if (targetPagePosition == ImmutableData::MemoryPages.end()) throw std::runtime_error(missingPage);
		// if the memory block has already been freed - an error
		const auto releasedBlock = "Specified block is already released.";
		if (block->IsReleased) throw std::runtime_error(releasedBlock);
		block->IsReleased = true;
		// free the memory block, and if the page is still occupied - exit
		auto targetPageBlocks = &(block->OwnerPage->MemoryBlocks);
		auto unreleasedBlockPosition = std::find_if(targetPageBlocks->begin(), targetPageBlocks->end(), [](MemoryBlock* b) { return !b->IsReleased; });
		if (unreleasedBlockPosition != targetPageBlocks->end()) return;
		// free the memory page and remove page link from cache
		auto targetPagePinter = block->OwnerPage;
		MemoryProtector::FreePage(targetPagePinter);
		ImmutableData::MemoryPages.erase(targetPagePosition);
		// remove memory blocks that corresponding to the page
		for (auto block : targetPagePinter->MemoryBlocks) delete block;
		targetPagePinter->MemoryBlocks.clear();
		delete targetPagePinter;
	};

	template<class T> std::list<MemoryPage*>::iterator ImmutableAllocator<T>::FindMemoryPagePositionWithEnoughSpace(size_t minFreeSpace)
	{
		if (ImmutableData::MemoryPages.size() == 0) return ImmutableData::MemoryPages.end();
		if (ImmutableData::MemoryPages.back()->TotalSize - ImmutableData::MemoryPages.back()->FillOffset < minFreeSpace) return ImmutableData::MemoryPages.end();
		for (auto it = ImmutableData::MemoryPages.begin(); it != ImmutableData::MemoryPages.end(); ++it)
			if ((*it)->TotalSize - (*it)->FillOffset >= minFreeSpace)
				return it;
		return ImmutableData::MemoryPages.end();
	};

	template<class T> std::list<MemoryBlock*>::iterator ImmutableAllocator<T>::FindMemoryBlockPositionByStartAddress(void* startAddress, size_t blockSize)
	{
		for (auto page : ImmutableData::MemoryPages)
			if ((page->StartAddress <= startAddress) && (((char*)startAddress + blockSize) <= ((char*)page->StartAddress + page->FillOffset)))
				for (auto blockPosition = page->MemoryBlocks.begin(); blockPosition != page->MemoryBlocks.end(); ++blockPosition)
					if (((*blockPosition)->StartAddress == startAddress) && ((*blockPosition)->TotalSize == blockSize))
						return blockPosition;
		const auto corruptedBlockOrder = "Memory blocks order is corrupted.";
		throw std::runtime_error(corruptedBlockOrder);
	};

	template<class T> std::list<MemoryBlock*>* ImmutableAllocator<T>::FindMemoryBlocksByStartAddress(void* startAddress, size_t blockSize, size_t blockCount)
	{
		auto blockPosition = FindMemoryBlockPositionByStartAddress(startAddress, blockSize);
		auto resultBlocks = new std::list<MemoryBlock*>();

		while (blockPosition != (*blockPosition)->OwnerPage->MemoryBlocks.end())
		{
			const auto corruptedBlockStatus = "Memory block status is corrupted.";
			if ((*blockPosition)->TotalSize != blockSize) throw std::runtime_error(corruptedBlockStatus);
			resultBlocks->push_back((*blockPosition));
			if (resultBlocks->size() == blockCount) return resultBlocks;
			++blockPosition;
		}

		const auto corruptedPageStatus = "Memory page status is corrupted.";
		throw std::runtime_error(corruptedPageStatus);
	};

	template<class T> void ImmutableAllocator<T>::InsertMemoryPageInCache(MemoryPage* page)
	{
		auto insertPagePosition = FindMemoryPagePositionWithEnoughSpace(page->TotalSize - page->FillOffset);
		if (insertPagePosition == ImmutableData::MemoryPages.end()) ImmutableData::MemoryPages.push_back(page);
		else ImmutableData::MemoryPages.insert(insertPagePosition, page);
	};
};