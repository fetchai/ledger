//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "vectorise/meta/log2.hpp"
#include <gtest/gtest.h>
#include <iostream>

#define TEST_LOG_EX__EQ__X(X) fetch::meta::Log2<(1ull << X)>::value == X

#define TEST_LOG_EX_PLUS_Y__EQ__X(X, Y) fetch::meta::Log2<(1ull << X) + Y>::value == X
TEST(vectorise_exact_exponents_gtest, zero_to_nine)
{
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(0));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(1));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(2));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(3));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(4));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(5));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(6));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(7));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(8));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(9));
}

TEST(vectorise_exact_exponents_gtest, ten_to_nineteen)
{
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(10));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(11));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(12));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(13));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(14));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(15));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(16));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(17));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(18));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(19));
}

TEST(vectorise_exact_exponents_gtest, twenty_to_twenty_nine)
{
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(20));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(21));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(22));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(23));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(24));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(25));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(26));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(27));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(28));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(29));
}

TEST(vectorise_exact_exponents_gtest, thirty_to_thirty_nine)
{
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(30));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(31));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(32));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(33));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(34));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(35));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(36));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(37));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(38));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(39));
}

TEST(vectorise_exact_exponents_gtest, fourty_to_fourty_nine)
{
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(40));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(41));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(42));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(43));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(44));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(45));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(46));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(47));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(48));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(49));
}

TEST(vectorise_exact_exponents_gtest, fifty_to_fifty_nine)
{
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(50));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(51));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(52));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(53));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(54));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(55));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(56));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(57));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(58));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(59));
}

TEST(vectorise_exact_exponents_gtest, sixty_to_sixty_three)
{
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(60));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(61));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(62));
  EXPECT_TRUE(TEST_LOG_EX__EQ__X(63));
}

TEST(vectorise_exact_exponents_gtest, Randomly_selected_tests)
{
  EXPECT_TRUE(TEST_LOG_EX_PLUS_Y__EQ__X(0, 0));
  EXPECT_TRUE(TEST_LOG_EX_PLUS_Y__EQ__X(1, 0));
  EXPECT_TRUE(TEST_LOG_EX_PLUS_Y__EQ__X(2, 1));
  EXPECT_TRUE(TEST_LOG_EX_PLUS_Y__EQ__X(3, 2));
  EXPECT_TRUE(TEST_LOG_EX_PLUS_Y__EQ__X(3, 2));
  EXPECT_TRUE(TEST_LOG_EX_PLUS_Y__EQ__X(3, 7));
  EXPECT_TRUE(TEST_LOG_EX_PLUS_Y__EQ__X(6, (1ull << 6) - 1));
}