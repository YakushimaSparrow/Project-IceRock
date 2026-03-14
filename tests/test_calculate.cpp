#include <gtest/gtest.h>
#include "R.hpp"

TEST(Calc,DrawdownNormal){std::vector<int> d={100,90,80,95,70};EXPECT_EQ(calculateMaxDragdown(d),30);}
TEST(Calc,DrawdownAsc){std::vector<int> d={10,20,30};EXPECT_EQ(calculateMaxDragdown(d),0);}
TEST(Calc,DrawdownSingle){std::vector<int> d={42};EXPECT_EQ(calculateMaxDragdown(d),0);}
TEST(Calc,DrawdownEmpty){EXPECT_EQ(calculateMaxDragdown({}),0);}
