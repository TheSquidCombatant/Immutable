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

	TEST(ImmutableAllocatorTests, SequenceMutableObjectChangeResultIsOk)
	{
		const char old_value = 'a';
		const char new_value = 'z';
		const int count = 100000;
		std::vector<char> chars;
		for (int i = 0; i < count; ++i) chars.push_back(old_value);
		for (int i = 0; i < count; ++i) ASSERT_EQ(chars[i], old_value);
		for (int i = 0; i < count; ++i) ASSERT_NO_THROW(chars[i] = new_value);
		for (int i = 0; i < count; ++i) ASSERT_EQ(chars[i], new_value);
	};

	TEST(ImmutableAllocatorTests, SequenceImmutableObjectChangeResultIsError)
	{
		const char old_value = 'a';
		const char new_value = 'z';
		const int count = 100000;
		std::vector<char, ImmutableAllocator<char>> chars;
		for (int i = 0; i < count; ++i) chars.push_back(old_value);
		for (int i = 0; i < count; ++i) ASSERT_EQ(chars[i], old_value);
		for (int i = 0; i < count; ++i) ASSERT_ANY_THROW(chars[i] = new_value);
		for (int i = 0; i < count; ++i) ASSERT_NE(chars[i], new_value);
	};
};