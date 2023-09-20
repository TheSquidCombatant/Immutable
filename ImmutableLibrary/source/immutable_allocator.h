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
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto block = CatchBlock(sizeof(T) * count_objects);
		return (T*)block->StartAddress;
	};

	template<class T> template<class U, class... Args> void ImmutableAllocator<T>::construct(U* p, Args&&... args)
	{
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto block = FindMemoryBlockByStartAddress(p);
		MemoryProtector::UnlockPage(block->OwnerPage);
		std::construct_at<U>(p, std::forward<Args>(args)...);
		MemoryProtector::LockPage(block->OwnerPage);
	};

	template<class T> template<class U> void ImmutableAllocator<T>::destroy(U* p)
	{
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto block = FindMemoryBlockByStartAddress(p);
		MemoryProtector::UnlockPage(block->OwnerPage);
		std::destroy_at<U>(p);
		MemoryProtector::LockPage(block->OwnerPage);
	}

	template<class T> void ImmutableAllocator<T>::deallocate(T* ptr, std::size_t count_objects)
	{
		const std::lock_guard<std::mutex> guard(ImmutableData::Mutex);
		auto block = FindMemoryBlockByStartAddress(ptr);
		FreeBlock(block);
	};

	template<class T> template<class U> bool ImmutableAllocator<T>::operator==(const ImmutableAllocator<U>& other)
	{
		return (this == other);
	};

	template<class T> template<class U> bool ImmutableAllocator<T>::operator!=(const ImmutableAllocator<U>& other)
	{
		return (this != other);
	};

	template<class T> MemoryBlock* ImmutableAllocator<T>::CatchBlock(size_t blockSize)
	{
		auto pageSize = ((blockSize % ImmutableData::SystemPageSize == 0) ? blockSize : (((blockSize / ImmutableData::SystemPageSize) + 1) * ImmutableData::SystemPageSize));
		auto targetPagePosition = FindMemoryPageWithEnoughSpace(blockSize);
		if (targetPagePosition == ImmutableData::MemoryPages.end())
		{
			auto page = MemoryProtector::CatchPage(pageSize);
			auto block = new MemoryBlock(page->StartAddress, blockSize, page);
			page->FillOffset = blockSize;
			page->MemoryBlocks.push_back(block);
			ImmutableData::MemoryPages.push_back(page);
			return block;
		}
		else
		{
			MemoryPage* page = (*targetPagePosition);
			auto block = new MemoryBlock((char*)page->StartAddress + page->FillOffset, blockSize, page);
			page->FillOffset += blockSize;
			page->MemoryBlocks.push_back(block);
			ImmutableData::MemoryPages.erase(targetPagePosition);
			InsertMemoryPageInCache(page);
			return block;
		}
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

	template<class T> std::list<MemoryPage*>::iterator ImmutableAllocator<T>::FindMemoryPageWithEnoughSpace(size_t minFreeSpace)
	{
		if (ImmutableData::MemoryPages.size() == 0) return ImmutableData::MemoryPages.end();
		if (ImmutableData::MemoryPages.back()->TotalSize - ImmutableData::MemoryPages.back()->FillOffset < minFreeSpace) return ImmutableData::MemoryPages.end();
		for (auto it = ImmutableData::MemoryPages.begin(); it != ImmutableData::MemoryPages.end(); ++it)
			if ((*it)->TotalSize - (*it)->FillOffset >= minFreeSpace)
				return it;
		return ImmutableData::MemoryPages.end();
	};

	template<class T> MemoryBlock* ImmutableAllocator<T>::FindMemoryBlockByStartAddress(void* startAddress)
	{
		for (auto page : ImmutableData::MemoryPages)
		{
			for (auto block : page->MemoryBlocks)
			{
				if (block->StartAddress == startAddress)
				{
					return block;
				}
			}
		}
		const auto corruptedBlockOrder = "Memory blocks order is corrupted.";
		throw std::runtime_error(corruptedBlockOrder);
	};

	template<class T> void ImmutableAllocator<T>::InsertMemoryPageInCache(MemoryPage* page)
	{
		auto insertPagePosition = FindMemoryPageWithEnoughSpace(page->TotalSize - page->FillOffset);
		if (insertPagePosition == ImmutableData::MemoryPages.end()) ImmutableData::MemoryPages.push_back(page);
		else ImmutableData::MemoryPages.insert(insertPagePosition, page);
	};
};