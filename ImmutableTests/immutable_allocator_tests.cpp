#include "gtest/gtest.h"

#include "..\ImmutableLibrary\headers\immutable.h"

namespace immutable::tests
{
	TEST(ImmutableAllocatorTests, SingleMutableObjectChangeResultIsOk)
	{
		const int old_value = INT_MAX;
		const int new_value = INT_MIN;
		ASSERT_NE(old_value, new_value);
		auto object = new int(old_value);
		ASSERT_EQ((*object), old_value);
		ASSERT_NO_THROW((*object) = new_value);
		ASSERT_EQ((*object), new_value);
		delete object;
	};

	TEST(ImmutableAllocatorTests, SingleImmutableObjectChangeResultIsError)
	{
		const int old_value = INT_MAX;
		const int new_value = INT_MIN;
		ASSERT_NE(old_value, new_value);
		auto allocator = ::immutable::ImmutableAllocator<int>();
		auto object = allocator.allocate(1);
		allocator.construct(object, old_value);
		ASSERT_EQ((*object), old_value);
		ASSERT_ANY_THROW((*object) = new_value);
		ASSERT_NE((*object), new_value);
		allocator.destroy(object);
		allocator.deallocate(object, 1);
	};
};