#include "gtest/gtest.h"
#include "gmock/gmock.h"


// Simple test, does not use gmock
TEST(Foo, foo)
{
    EXPECT_EQ(1, 1);
}
