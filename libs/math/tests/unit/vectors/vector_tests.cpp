//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "math/standard_functions/log.hpp"
#include "math/trigonometry.hpp"
#include "test_types.hpp"
#include "vectorise/math/standard_functions.hpp"
#include "vectorise/vectorise.hpp"

#include "gtest/gtest.h"

#include <cstdlib>
#include <functional>
#include <memory>

namespace fetch {
namespace math {
namespace test {

using namespace fetch::vectorise;

#ifdef __AVX2__
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
#endif

template <typename T>
class VectorRegisterTest : public ::testing::Test
{
};

#ifdef __AVX2__
using MyTypes = ::testing::Types<
    fetch::vectorise::VectorRegister<float, 128>, fetch::vectorise::VectorRegister<float, 256>,
    fetch::vectorise::VectorRegister<int32_t, 128>, fetch::vectorise::VectorRegister<int32_t, 256>,
    fetch::vectorise::VectorRegister<int64_t, 128>, fetch::vectorise::VectorRegister<int64_t, 256>,
    fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 128>,
    fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 256>,
    fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 128>,
    fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 256>,
    fetch::vectorise::VectorRegister<double, 128>, fetch::vectorise::VectorRegister<double, 256>>;

using MyFPTypes =
    ::testing::Types<fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 128>,
                     fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 256>,
                     fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 128>,
                     fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 256>>;
#else
using MyTypes = ::testing::Types<fetch::vectorise::VectorRegister<float, 32>,
                                 fetch::vectorise::VectorRegister<int32_t, 32>,
                                 fetch::vectorise::VectorRegister<int64_t, 64>,
                                 fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 32>,
                                 fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 64>,
                                 fetch::vectorise::VectorRegister<double, 64>>;

using MyFPTypes =
    ::testing::Types<fetch::vectorise::VectorRegister<fetch::fixed_point::fp32_t, 32>,
                     fetch::vectorise::VectorRegister<fetch::fixed_point::fp64_t, 64>>;
#endif

TYPED_TEST_CASE(VectorRegisterTest, MyTypes);
TYPED_TEST(VectorRegisterTest, basic_tests)
{
  using type = typename TypeParam::type;

  alignas(32) type a[TypeParam::E_BLOCK_COUNT], b[TypeParam::E_BLOCK_COUNT],
      sum[TypeParam::E_BLOCK_COUNT], diff[TypeParam::E_BLOCK_COUNT], prod[TypeParam::E_BLOCK_COUNT],
      div[TypeParam::E_BLOCK_COUNT];

  type real_max{fetch::math::Type<type>("0")}, real_min{fetch::math::numeric_max<type>()};
  for (std::size_t i = 0; i < TypeParam::E_BLOCK_COUNT; i++)
  {
    // We don't want to check overflows right now, so we pick std::rand numbers, but well within the
    // type's limits
    a[i] = fetch::math::Type<type>(
        std::to_string((static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX)) *
                       static_cast<double>(fetch::math::numeric_max<type>()) / 2.0));
    b[i] = fetch::math::Type<type>(
        std::to_string((static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX)) *
                       static_cast<double>(fetch::math::numeric_max<type>()) / 2.0));
    sum[i]   = a[i] + b[i];
    diff[i]  = a[i] - b[i];
    prod[i]  = a[i] * b[i];
    div[i]   = a[i] / b[i];
    real_max = fetch::vectorise::Max(a[i], real_max);
    real_max = fetch::vectorise::Max(b[i], real_max);
    real_min = fetch::vectorise::Min(a[i], real_min);
    real_min = fetch::vectorise::Min(b[i], real_min);
  }
  TypeParam va{a};
  TypeParam vb{b};

  auto vsum  = va + vb;
  auto vdiff = va - vb;
  auto vprod = va * vb;
  auto vdiv  = va / vb;

  TypeParam vtmp1{sum}, vtmp2{diff}, vtmp3{prod}, vtmp4{div};
  EXPECT_TRUE(all_equal_to(vtmp1, vsum));
  EXPECT_TRUE(all_equal_to(vtmp2, vdiff));
  EXPECT_TRUE(all_equal_to(vtmp3, vprod));
  EXPECT_TRUE(all_equal_to(vtmp4, vdiv));

  type reduce1 = reduce(vsum);
  type hsum{0};
  for (std::size_t i = 0; i < TypeParam::E_BLOCK_COUNT; i++)
  {
    hsum += sum[i];
  }
  EXPECT_EQ(hsum, reduce1);

  TypeParam vmax = Max(va, vb);
  type      max  = Max(vmax);
  EXPECT_EQ(max, real_max);

  TypeParam vmin = Min(va, vb);
  type      min  = Min(vmin);
  EXPECT_EQ(min, real_min);
}

template <typename T>
class VectorReduceTest : public ::testing::Test
{
};

TYPED_TEST_CASE(VectorReduceTest, MyFPTypes);
TYPED_TEST(VectorReduceTest, reduce_tests)
{
  using type       = typename TypeParam::type;
  using array_type = fetch::memory::SharedArray<type>;

  std::size_t            N = 20, offset = 2;
  alignas(32) array_type A(N), B(N), C(N), D(N), E(N);
  type sum{0}, partial_sum{0}, max_a{fetch::math::Type<type>("0")}, min_a{type(N)}, partial_max{0},
      partial_min{type(N)};

  for (std::size_t i = 0; i < N; ++i)
  {
    A[i] = fetch::math::Sin(fetch::math::Type<type>("-0.1") * type(i));
    B[i] = fetch::math::Sin(fetch::math::Type<type>("0.1") * type(i + 1));
    sum += A[i] + B[i];
    max_a = fetch::vectorise::Max(A[i], max_a);
    min_a = fetch::vectorise::Min(A[i], min_a);
    if (i >= offset && i < N - offset)
    {
      partial_sum += A[i] + B[i];
      partial_max = fetch::vectorise::Max(A[i], partial_max);
      partial_min = fetch::vectorise::Min(A[i], partial_min);
    }
  }

  type ret;
  ret = A.in_parallel().Reduce(
      [](auto const &a, auto const &b) { return fetch::vectorise::Max(a, b); },
      [](auto const &a) { return fetch::vectorise::Max(a); });
  EXPECT_EQ(ret, max_a);

  fetch::memory::Range range(2, A.size() - 2);
  ret = A.in_parallel().Reduce(
      range, [](auto const &a, auto const &b) { return fetch::vectorise::Max(a, b); },
      [](auto const &a) { return fetch::vectorise::Max(a); });
  EXPECT_EQ(ret, partial_max);

  ret = A.in_parallel().Reduce(
      [](auto const &a, auto const &b) { return fetch::vectorise::Min(a, b); },
      [](auto const &a) { return fetch::vectorise::Min(a); }, type(N * N));
  EXPECT_EQ(ret, min_a);

  ret = A.in_parallel().Reduce(
      range, [](auto const &a, auto const &b) { return fetch::vectorise::Min(a, b); },
      [](auto const &a) { return fetch::vectorise::Min(a); }, type(N * N));
  EXPECT_EQ(ret, partial_min);

  ret = A.in_parallel().SumReduce([](auto const &a, auto const &b) { return a + b; }, B);
  EXPECT_EQ(ret, sum);

  ret = A.in_parallel().SumReduce(range, [](auto const &a, auto const &b) { return a + b; }, B);
  EXPECT_EQ(ret, partial_sum);

  C.in_parallel().Apply([](auto const &a, auto const &b, auto &c) { c = a + b; }, A, B);

  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_EQ(C[i], A[i] + B[i]);
  }

  type const           beta(4);
  fetch::memory::Range small_range(6, 15);
  C.in_parallel().RangedApplyMultiple(
      small_range,
      [beta](auto const &a, auto const &b, auto &c) {
        c = a * static_cast<std::remove_reference_t<decltype(a)>>(beta) + b;
      },
      A, B);

  for (std::size_t i = 6; i < 15; ++i)
  {
    EXPECT_EQ(C[i], A[i] * beta + B[i]);
  }

  // Assign tests, assign all of B to C
  C.in_parallel().Apply([](auto const &a, auto &c) { c = a; }, B);

  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_EQ(C[i], B[i]);
  }

  // Assign range (6,15) of A to C
  C.in_parallel().RangedApplyMultiple(small_range, [](auto const &a, auto &c) { c = a; }, A);

  for (std::size_t i = 6; i < 15; ++i)
  {
    EXPECT_EQ(C[i], A[i]);
  }

  D.in_parallel().Assign(beta);
  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_EQ(D[i], beta);
  }

  E.in_parallel().Assign(C);
  for (std::size_t i = 0; i < N; ++i)
  {
    EXPECT_EQ(E[i], C[i]);
  }
}

template <typename T>
class VectorNaNInfTest : public ::testing::Test
{
};
TYPED_TEST_CASE(VectorNaNInfTest, MyFPTypes);
TYPED_TEST(VectorNaNInfTest, nan_inf_tests)
{
  using type = typename TypeParam::type;

  alignas(32) type PInf[TypeParam::E_BLOCK_COUNT], NInf[TypeParam::E_BLOCK_COUNT],
      NaN[TypeParam::E_BLOCK_COUNT], A[TypeParam::E_BLOCK_COUNT], B[TypeParam::E_BLOCK_COUNT],
      C[TypeParam::E_BLOCK_COUNT], D[TypeParam::E_BLOCK_COUNT];

  for (size_t i = 0; i < TypeParam::E_BLOCK_COUNT; i++)
  {
    PInf[i] = type::POSITIVE_INFINITY;
    NInf[i] = type::NEGATIVE_INFINITY;
    NaN[i]  = type::NaN;
    A[i]    = fetch::math::Sin(fetch::math::Type<type>("-0.1") * type(i));
    B[i]    = fetch::math::Sin(fetch::math::Type<type>("0.1") * type(i + 1));
    C[i]    = A[i] + B[i];
  }
  TypeParam vpos_inf{PInf};
  TypeParam vneg_inf{NInf};
  TypeParam vnan{NaN};
  TypeParam va{A};
  TypeParam vb{B};
  TypeParam vmax(type::FP_MAX);
  TypeParam vmin(type::FP_MIN);
  TypeParam vzero(type::_0);
  // NaN != NaN
  EXPECT_FALSE(any_equal_to(vnan, vnan));
  // -inf == -inf
  EXPECT_TRUE(all_equal_to(vneg_inf, vneg_inf));
  // +inf == +inf
  EXPECT_TRUE(all_equal_to(vpos_inf, vpos_inf));
  // -inf < +inf
  EXPECT_TRUE(all_less_than(vneg_inf, vpos_inf));
  // MAX < +inf
  EXPECT_TRUE(all_less_than(vmax, vpos_inf));
  // -inf < MIN
  EXPECT_TRUE(all_less_than(vneg_inf, vmin));
  // -inf < 0
  EXPECT_TRUE(all_less_than(vneg_inf, vzero));

  TypeParam vret;
  struct Test
  {
    std::function<void(size_t i)> scalar_set;
    std::function<void()>         vector_set;
    bool                          stateNaN;
    bool                          stateInf;
    bool                          stateOverflow;
  };
  std::vector<Test> tests = {
      {// Normal state check
       [&](size_t i) { C[i] = A[i] + B[i]; }, [&]() { vret = va + vb; }, false, false, false},
      {// Overflow state check
       [&](size_t i) {
         A[i] = type::FP_MAX * type::_half + type{i + 1};
         B[i] = type::FP_MAX * type::_half;
         C[i] = A[i] + B[i];
       },
       [&]() {
         va   = TypeParam(A);
         vb   = TypeParam(B);
         vret = va + vb;
       },
       false, false, true},
      {// Underflow state check
       [&](size_t i) {
         A[i] = -type::FP_MAX * type::_half - type{i - 1};
         B[i] = -type::FP_MAX * type::_half;
         C[i] = A[i] + B[i];
       },
       [&]() {
         va   = TypeParam(A);
         vb   = TypeParam(B);
         vret = va + vb;
       },
       false, false, true},
      {// Infinity state check, A +inf
       [&](size_t i) { C[i] = A[i] + PInf[i]; }, [&]() { vret = va + vpos_inf; }, false, true,
       false},
      {// Infinity state check, A + (-inf)
       [&](size_t i) { C[i] = A[i] + NInf[i]; }, [&]() { vret = va + vneg_inf; }, false, true,
       false},
      {// Infinity state check, +inf + B
       [&](size_t i) { C[i] = PInf[i] + B[i]; }, [&]() { vret = vpos_inf + vb; }, false, true,
       false},
      {// Infinity state check, -inf +B
       [&](size_t i) { C[i] = NInf[i] + B[i]; }, [&]() { vret = vneg_inf + vb; }, false, true,
       false},
      {// Infinity state check, (+inf) + (+inf)
       [&](size_t i) { C[i] = PInf[i] + PInf[i]; }, [&]() { vret = vpos_inf + vpos_inf; }, false,
       true, false},
      {// Infinity state check, (-inf) + (-inf)
       [&](size_t i) { C[i] = NInf[i] + NInf[i]; }, [&]() { vret = vneg_inf + vneg_inf; }, false,
       true, false},
      {// NaN state check, +inf + (-inf)
       [&](size_t i) { C[i] = PInf[i] + NInf[i]; }, [&]() { vret = vpos_inf + vneg_inf; }, true,
       false, false},
      {// NaN state check, +inf + (-inf)
       [&](size_t i) { C[i] = NInf[i] + PInf[i]; }, [&]() { vret = vneg_inf + vpos_inf; }, true,
       false, false},
      {// Infinity state check, (+inf) + nan
       [&](size_t i) { C[i] = PInf[i] + NaN[i]; }, [&]() { vret = vpos_inf + vnan; }, true, false,
       false},
      {// Infinity state check, (-inf) + (+inf)
       [&](size_t i) { C[i] = NInf[i] + NaN[i]; }, [&]() { vret = vneg_inf + vnan; }, true, false,
       false},
      {// Infinity state check, nan + (+inf)
       [&](size_t i) { C[i] = NaN[i] + PInf[i]; }, [&]() { vret = vnan + vpos_inf; }, true, false,
       false},
      {// Infinity state check, (-inf) + (+inf)
       [&](size_t i) { C[i] = NaN[i] + NInf[i]; }, [&]() { vret = vnan + vneg_inf; }, true, false,
       false},
      {// Overflow state check, A * B
       [&](size_t i) { C[i] = A[i] * B[i]; }, [&]() { vret = va * vb; }, false, false, true},
      {// Infinity state check, A * (+inf)
       [&](size_t i) { C[i] = A[i] * PInf[i]; }, [&]() { vret = va * vpos_inf; }, false, true,
       false},
      {// Infinity state check, A * (-inf)
       [&](size_t i) { C[i] = A[i] * NInf[i]; }, [&]() { vret = va * vneg_inf; }, false, true,
       false},
      {// NaN state check, A * nan
       [&](size_t i) { C[i] = A[i] * NaN[i]; }, [&]() { vret = va * vnan; }, true, false, false}};

  size_t j = 0;
  for (j = 0; j < tests.size(); j++)
  {
    type::StateClear();
    for (size_t i = 0; i < TypeParam::E_BLOCK_COUNT; i++)
    {
      tests[j].scalar_set(i);
    }
    EXPECT_EQ(type::IsStateNaN(), tests[j].stateNaN);
    EXPECT_EQ(type::IsStateInfinity(), tests[j].stateInf);
    EXPECT_EQ(type::IsStateOverflow(), tests[j].stateOverflow);
    type::StateClear();
    tests[j].vector_set();
    EXPECT_EQ(type::IsStateNaN(), tests[j].stateNaN);
    EXPECT_EQ(type::IsStateInfinity(), tests[j].stateInf);
    EXPECT_EQ(type::IsStateOverflow(), tests[j].stateOverflow);
    vret.Store(D);
    for (size_t i = 0; i < TypeParam::E_BLOCK_COUNT; i++)
    {
      if (!type::IsNaN(C[i]) && !type::IsNaN(D[i]))
      {
        EXPECT_EQ(C[i], D[i]);
      }
    }
  }
}

}  // namespace test
}  // namespace math
}  // namespace fetch
