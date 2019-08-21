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
#include "math/trigonometry.hpp"
#include "vectorise/math/standard_functions.hpp"
#include "vectorise/vectorise.hpp"

#include "gtest/gtest.h"

#include <functional>

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

#ifdef __AVX2__
using MyTypes = ::testing::Types< fetch::vectorise::VectorRegister<float, 128>,
                                  fetch::vectorise::VectorRegister<float, 256>, 
                                  fetch::vectorise::VectorRegister<int32_t, 128>,
                                  fetch::vectorise::VectorRegister<int32_t, 256>,
                                  fetch::vectorise::VectorRegister<int64_t, 128>,
                                  fetch::vectorise::VectorRegister<int64_t, 256>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 128>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 256>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 128>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 256>,
                                  fetch::vectorise::VectorRegister<double, 128>,
                                  fetch::vectorise::VectorRegister<double, 256>>;

using MyFPTypes = ::testing::Types<fetch::vectorise::VectorRegister<float, 256>, 
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 256>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 256>,
                                  fetch::vectorise::VectorRegister<double, 256>>;
#else
using MyTypes = ::testing::Types< fetch::vectorise::VectorRegister<float, 32>,
                                  fetch::vectorise::VectorRegister<int32_t, 32>,
                                  fetch::vectorise::VectorRegister<int64_t, 64>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 32>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 64>,
                                  fetch::vectorise::VectorRegister<double, 64>>;

using MyFPTypes = ::testing::Types< fetch::vectorise::VectorRegister<float, 32>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 32>,
                                  fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 64>,
                                  fetch::vectorise::VectorRegister<double, 64>>;
#endif


TYPED_TEST_CASE(VectorRegisterTest, MyTypes);
TYPED_TEST(VectorRegisterTest, basic_tests)
{
  using type = typename TypeParam::type;

  alignas(TypeParam::E_REGISTER_SIZE) type  a[TypeParam::E_BLOCK_COUNT], b[TypeParam::E_BLOCK_COUNT],
                                            sum[TypeParam::E_BLOCK_COUNT], diff[TypeParam::E_BLOCK_COUNT],
                                            prod[TypeParam::E_BLOCK_COUNT], div[TypeParam::E_BLOCK_COUNT];
  for (size_t i = 0; i < TypeParam::E_BLOCK_COUNT; i++)
  {
    // We don't want to check overflows right now, so we pick random numbers, but well within the type's limits
    a[i] = type(double(random()) / (double)(RAND_MAX) * (1 << (TypeParam::E_REGISTER_SIZE/2)) );
    b[i] = type(double(random()) / (double)(RAND_MAX) * (1 << (TypeParam::E_REGISTER_SIZE/2)) + 1 );
    sum[i] = a[i] + b[i];
    diff[i] = a[i] - b[i];
    prod[i] = a[i] * b[i];
    div[i] = a[i] / b[i];
  }
  TypeParam va{a};
  TypeParam vb{b};

  auto vsum = va + vb;
  auto vdiff = va - vb;
  auto vprod = va * vb;
  auto vdiv = va / vb;

  TypeParam vtmp1{sum}, vtmp2{diff}, vtmp3{prod}, vtmp4{div};
  EXPECT_TRUE(all_equal_to(vtmp1, vsum));
  EXPECT_TRUE(all_equal_to(vtmp2, vdiff));  
  EXPECT_TRUE(all_equal_to(vtmp3, vprod));
  EXPECT_TRUE(all_equal_to(vtmp4, vdiv));

  type reduce1 = reduce(vsum);
  type reduce2 = reduce(vdiff);
  type reduce3 = reduce(vprod);
  type reduce4 = reduce(vdiv);

  std::cout << "va  = " << va << std::endl;
  std::cout << "vb  = " << vb << std::endl;
  std::cout << "vsum  = " << vsum << std::endl;
  std::cout << "vdiff = " << vdiff << std::endl;
  std::cout << "vprod = " << vprod << std::endl;
  std::cout << "vdiv  = " << vdiv << std::endl;
  std::cout << "reduce(vsum)  = " << reduce1 << std::endl;
  std::cout << "reduce(vdiff) = " << reduce2 << std::endl;
  std::cout << "reduce(vprod) = " << reduce3 << std::endl;
  std::cout << "reduce(vdiv)  = " << reduce4 << std::endl;

  TypeParam vmax = Max(va, vb);
  std::cout << "vmax  = " << vmax << std::endl;
  type max = Max(vmax);
  std::cout << "max  = " << max << std::endl;
}

template <typename T>
class VectorReduceTest : public ::testing::Test
{
};

TYPED_TEST_CASE(VectorReduceTest, MyFPTypes);
TYPED_TEST(VectorReduceTest, reduce_tests)
{
  using type = typename TypeParam::type;
  using array_type  = fetch::memory::SharedArray<type>;
  using vector_type = typename array_type::VectorRegisterType;

  std::size_t N = 60;
  alignas(TypeParam::E_REGISTER_SIZE) array_type A(N), B(N);
  type sum{0}, max_a{type(0)}, min_a{type(N)};

  for (std::size_t i = 0; i < N; ++i)
  {
    A[i] = type((i+1)*4);
    B[i] = type((i+1)*(i+1));
    std::cout << "A[" << i << "] = " << A[i] << std::endl;
    std::cout << "B[" << i << "] = " << B[i] << std::endl;
    sum += A[i] + B[i];
    max_a = fetch::vectorise::Max(A[i], max_a);
    min_a = fetch::vectorise::Min(A[i], min_a);
  }
  std::cout << "Sum = " << sum << std::endl;
  std::cout << "Max = " << max_a << std::endl;
  std::cout << "Min = " << min_a << std::endl;

  type ret;
  ret = A.in_parallel().Reduce(
        [](auto const &a, auto const &b) {
          return fetch::vectorise::Max(a, b);
        }, 
        [](vector_type const &a) {
          return fetch::vectorise::Max(a);
        });
  std::cout << "Reduce: Max = " << ret << std::endl;

  fetch::memory::Range range(2, A.size() -2);
  ret = A.in_parallel().Reduce(range,
        [](auto const &a, auto const &b) {
          return fetch::vectorise::Max(a, b);
        }, 
        [](vector_type const &a) {
          return fetch::vectorise::Max(a);
        });
  std::cout << "Reduce (range: 2, N-2): Max = " << ret << std::endl;

  ret = A.in_parallel().Reduce(
        [](auto const &a, auto const &b) {
          return fetch::vectorise::Min(a, b);
        }, 
        [](vector_type const &a) {
          return fetch::vectorise::Min(a);
        }, type(N*N));
  std::cout << "Reduce: Min = " << ret << std::endl;

  ret = A.in_parallel().Reduce(range,
        [](auto const &a, auto const &b) {
          return fetch::vectorise::Min(a, b);
        }, 
        [](vector_type const &a) {
          return fetch::vectorise::Min(a);
        }, type(N*N));
  std::cout << "Reduce (range: 2, N-2): Min = " << ret << std::endl;

  ret = A.in_parallel().SumReduce([](auto const &a, auto const &b) {
          return a + b;
        }, 
        [](vector_type const &a) {
          return reduce(a);
        }, B);
  std::cout << "SumReduce: ret = " << ret << std::endl;

  // ret = A.in_parallel().ProductReduce([](auto const &a, auto const &b) {
  //       return a * b;
  //       }, B);

  // std::cout << "ProductReduce: ret = " << ret << std::endl;

  
  // ret = A.in_parallel().GenericReduce(range, std::plus<vector_type>{}, [](auto const &a, auto const &b) {
  //       return a * b;
  //       },
  //       [](vector_type const &vr_a_i) {
  //           return reduce(vr_a_i);
  //       },
  //       type(0), B);
  // std::cout << "GenericReduce(range): ret = " << ret << std::endl;
}