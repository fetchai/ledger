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

#include "math/base_types.hpp"
#include "vectorise/vectorise.hpp"

#include "gtest/gtest.h"

using namespace fetch::vectorise;

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

template <typename T>
class VectorRegisterTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types< fetch::vectorise::VectorRegister<float, 128>,
                                  fetch::vectorise::VectorRegister<float, 256>, 
                                  fetch::vectorise::VectorRegister<int32_t, 128>,
                                  fetch::vectorise::VectorRegister<int32_t, 256>,
                                  fetch::vectorise::VectorRegister<int64_t, 128>,
                                  fetch::vectorise::VectorRegister<int64_t, 256>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 128>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 256>,
                                  fetch::vectorise::VectorRegister<double, 128>,
                                  fetch::vectorise::VectorRegister<double, 256>>;

TYPED_TEST_CASE(VectorRegisterTest, MyTypes);
TYPED_TEST(VectorRegisterTest, basic_tests)
{
  std::cout << "TypeParam::E_BLOCK_COUNT   = " << TypeParam::E_BLOCK_COUNT << std::endl;
  std::cout << "TypeParam::E_REGISTER_SIZE = " << TypeParam::E_REGISTER_SIZE << std::endl;
  alignas(TypeParam::E_REGISTER_SIZE) typename TypeParam::type  a[TypeParam::E_BLOCK_COUNT], b[TypeParam::E_BLOCK_COUNT],
                                                              sum[TypeParam::E_BLOCK_COUNT], diff[TypeParam::E_BLOCK_COUNT],
                                                              prod[TypeParam::E_BLOCK_COUNT], div[TypeParam::E_BLOCK_COUNT];
  for (size_t i = 0; i < TypeParam::E_BLOCK_COUNT; i++)
  {
    // We don't want to check overflows right now, so we pick random numbers, but well within the type's limits
    a[i] = typename TypeParam::type(double(random()) / (double)(RAND_MAX) * (1 << TypeParam::E_REGISTER_SIZE/2) );
    b[i] = typename TypeParam::type(double(random()) / (double)(RAND_MAX) * (1 << TypeParam::E_REGISTER_SIZE/2) );
    sum[i] = a[i] + b[i];
    diff[i] = a[i] - b[i];
    prod[i] = a[i] * b[i];
    div[i] = a[i] / b[i];
    std::cout << "sum[" << i << "]  = " << sum[i] << std::endl;
    std::cout << "diff[" << i << "]  = " << diff[i] << std::endl;
    std::cout << "prod[" << i << "]  = " << prod[i] << std::endl;
    std::cout << "div[" << i << "]  = " << div[i] << std::endl;
  }
  TypeParam va{a};
  TypeParam vb{b};

  std::cout << va << std::endl;
  std::cout << vb << std::endl;

  auto vsum = va + vb;
  auto vdiff = va - vb;
  auto vprod = va * vb;
  auto vdiv = va / vb;
  std::cout << "sum  = " << vsum << std::endl;
  std::cout << "diff = " << vdiff << std::endl;
  std::cout << "prod = " << vprod << std::endl;
  std::cout << "div  = " << vdiv << std::endl;

  TypeParam vtmp1{sum}, vtmp2{diff}, vtmp3{prod}, vtmp4{div};
  EXPECT_TRUE(all_equal_to(vtmp1, vsum));
  EXPECT_TRUE(all_equal_to(vtmp2, vdiff));  
  EXPECT_TRUE(all_equal_to(vtmp3, vprod));
  EXPECT_TRUE(all_equal_to(vtmp4, vdiv));
}