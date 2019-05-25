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

#include "vectorise/register.hpp"
#include "vectorise/arch/sse.hpp"
#include <iostream>

#include <gtest/gtest.h>

using namespace fetch::vectorize;

TEST(vectorise_sse_gtest, register_test1)
{
  alignas(16) int a[4] = {1, 2, 3, 4};
  alignas(16) int b[4] = {2, 4, 8, 16};
  alignas(16) int c[4] = {0};

  VectorRegister<int, 128> r1(a), r2(b), r3;

  r3 = r1 * r2;
  r3 = r3 - r1;
  r3.Store(c);

  EXPECT_EQ(c[0], 1);
  EXPECT_EQ(c[1], 6);
  EXPECT_EQ(c[2], 21);
  EXPECT_EQ(c[3], 60);
}

TEST(vectorise_sse_gtest, register_test2)
{
  alignas(16) float a[4] = {1, 2, 3, 4};
  alignas(16) float b[4] = {2, 4, 8, 16};
  alignas(16) float c[4] = {0};

  VectorRegister<float, 128> r1(a), r2(b), r3, cst(3);

  r3 = r1 * r2;
  r3 = cst * r3 - r1;
  r3.Store(c);

  EXPECT_EQ(c[0], 5);
  EXPECT_EQ(c[1], 22);
  EXPECT_EQ(c[2], 69);
  EXPECT_EQ(c[3], 188);
}

TEST(vectorise_sse_gtest, register_test3)
{
  alignas(16) double a[2] = {1, 2};
  alignas(16) double b[2] = {2, 4};
  alignas(16) double c[2] = {0};

  VectorRegister<double, 128> r1(a), r2(b), r3, cst(3.2);

  r3 = r1 * r2;
  r3 = cst * r3 - r1;
  r3.Store(c);

  EXPECT_EQ(c[0], 5.4);
  EXPECT_EQ(c[1], 23.6);
}
