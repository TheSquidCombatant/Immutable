#pragma once

#include "..\headers\immutable.h"

namespace immutable
{
	template<class T> ImmutableGuard<T>::ImmutableGuard(T* wrappedObject)
	{
		ImmutableAllocator<T> allocator = ImmutableAllocator<T>();
		auto block = allocator.FindMemoryBlocksAndReturnFirst(wrappedObject, sizeof(T), 1);
		WrappedObject = (T*)block->StartAddress;
	};

	template<class T> template<class... Args> ImmutableGuard<T>::ImmutableGuard(Args&&... args)
	{
		static_assert(is_constructible_v<T, Args...>, "The required constructor was not found.");
		ImmutableAllocator<T> allocator = ImmutableAllocator<T>();
		WrappedObject = allocator.allocate(1);
		allocator.construct(WrappedObject, forward<Args>(args)...);
	};

	template<class T> ImmutableGuard<T>::~ImmutableGuard()
	{
		ImmutableAllocator<T> allocator = ImmutableAllocator<T>();
		allocator.destroy(WrappedObject);
		allocator.deallocate(WrappedObject, 1);
	};
};