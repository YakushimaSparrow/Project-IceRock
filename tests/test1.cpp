#include <gtest/gtest.h>

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "helo");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
    //TEST89
}