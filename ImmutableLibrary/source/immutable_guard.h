#pragma once

#include "..\headers\immutable.h"

namespace immutable
{
	template<class T> ImmutableGuard<T>::ImmutableGuard(T* wrappedObject)
	{
		ImmutableAllocator<T> allocator = ImmutableAllocator<T>();
		auto block = allocator.FindMemoryBlockByStartAddress(wrappedObject);
		WrappedObject = wrappedObject;
	};

	template<class T> template<class... Args> ImmutableGuard<T>::ImmutableGuard(Args&&... args)
	{
		ImmutableAllocator<T> allocator = ImmutableAllocator<T>();
		WrappedObject = allocator.allocate(1);
		allocator.construct(WrappedObject, std::forward<Args>(args)...);
	};

	template<class T> ImmutableGuard<T>::~ImmutableGuard()
	{
		ImmutableAllocator<T> allocator = ImmutableAllocator<T>();
		allocator.destroy(WrappedObject);
		allocator.deallocate(WrappedObject, 1);
	};
};