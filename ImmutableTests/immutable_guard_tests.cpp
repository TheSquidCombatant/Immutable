#include "gtest/gtest.h"

#include "..\ImmutableLibrary\headers\immutable.h"

namespace immutable::tests
{
	TEST(ImmutableGuardTests, GuardInitializationWithMutableObjectResultIsError)
	{
		const int value = INT_MAX;
		auto object = new int(value);
		ImmutableGuard<int>* guard = nullptr;
		ASSERT_ANY_THROW(guard = new ImmutableGuard<int>(object));
		ASSERT_EQ(guard, nullptr);
		delete object;
	};

	TEST(ImmutableGuardTests, GuardInitializationWithImmutableObjectResultIsOk)
	{
		const int old_value = INT_MAX;
		const int new_value = INT_MIN;
		ASSERT_NE(old_value, new_value);
		auto object = ImmutableAllocator<int>::allocate(1);
		ImmutableAllocator<int>::construct(object, old_value);
		ImmutableGuard<int>* guard;
		ASSERT_NO_THROW(guard = new ImmutableGuard<int>(object));
		ASSERT_EQ(*(guard->WrappedObject), old_value);
		ASSERT_ANY_THROW(*(guard->WrappedObject) = new_value);
		ASSERT_NE(*(guard->WrappedObject), new_value);
		ASSERT_NO_THROW(delete guard);
	};

	TEST(ImmutableGuardTests, GuardInitializationUsingThroughConstructorResultIsOk)
	{
		const int old_value = INT_MAX;
		const int new_value = INT_MIN;
		ASSERT_NE(old_value, new_value);
		ImmutableGuard<int>* guard;
		ASSERT_NO_THROW(guard = new ImmutableGuard<int>(old_value));
		ASSERT_EQ(*(guard->WrappedObject), old_value);
		ASSERT_ANY_THROW(*(guard->WrappedObject) = new_value);
		ASSERT_NE(*(guard->WrappedObject), new_value);
		ASSERT_NO_THROW(delete guard);
	};
};