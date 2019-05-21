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

#include "meta/log2.hpp"
#include <gtest/gtest.h>
#include <iostream>

namespace fetch {
namespace meta {
namespace {

using namespace testing;

/**
 * This Test classes is dedicated to test both - COMPILE & RUN time behaviour of
 * `T Log2(T val)` constexpr function.
 */
class LogTest : public Test
{
protected:
  template <typename T>
  T RuntimeLog2(T val)
  {
    auto res{fetch::meta::Log2(val)};
    return res;
  }

  /**
   * This class ensures that Log2(...) is exercised at COMPILE-time
   */
  template <typename T, T VAL, T LOG2 = fetch::meta::Log2(VAL)>
  struct Compiletime
  {
    static constexpr T Log2{LOG2};
  };

  /**
   * @brief This tests both - COMPILE & RUN time behaviour of Log2(...)
   *
   * This method is testing the following statement to be TRUE:
   * `EXPECTED_SUCCESS == (BIT_SHIFT == Log2((T{1} << BIT_SHIFT) + PLUS_VAL))`
   *
   * Based on above definition, the
   * `BIT_SHIFT == Log2((T{1} << BIT_SHIFT) + PLUS_VAL)` can be `true` !!!ONLY
   * and ONLY IF!!! `PLUS_VAL < (T{1} << BIT_SHIFT)`
   * Thus if `PLUS_VAL == (T{1} << BIT_SHIFT)`, it shall result to `false`.
   *
   * @tparam T type of input integer value of `T Log2(T value)` function.
   * @tparam BIT_SHIFT number of bites to shift 1 left to get the input value
   *         for Log2(...) function.
   * @tparam PLUS_VAL \anchor test_PLUS_VAL value to be added to (1 << BIT_SHIFT),
   *         defaults to 0
   * @tparam EXPECTED_SUCCESS  boolean compile time value representing whether
   *         test is expected to pass successfully or fail, defaults to `true`.
   */
  template <typename T, T BIT_SHIFT, T PLUS_VAL = 0, bool EXPECTED_SUCCESS = true>
  void test()
  {
    static constexpr T val{(T{1} << BIT_SHIFT) + PLUS_VAL};

    auto const  runtime_log2{RuntimeLog2(val)};
    constexpr T compiletime_log2{Compiletime<T, val>::Log2};

    if (EXPECTED_SUCCESS)
    {
      EXPECT_EQ(BIT_SHIFT, runtime_log2);
      EXPECT_EQ(BIT_SHIFT, compiletime_log2);
    }
    else
    {
      EXPECT_NE(BIT_SHIFT, runtime_log2);
      EXPECT_NE(BIT_SHIFT, compiletime_log2);
    }
  }

  /**
   * This tests expected to pass (because `PLUS_VAL = (T{1} << BIT_SHIFT) - 1`,
   * what is !!LESS THAN!! `T{1} << BIT_SHIFT`
   *
   * \ref test_PLUS_VAL
   */
  template <typename T, T BIT_SHIFT>
  void test_with_plus()
  {
    test<T, BIT_SHIFT, (T{1} << BIT_SHIFT) - 1>();
  }

  /**
   * This tests expected to fail (because `PLUS_VAL = (T{1} << BIT_SHIFT)`,
   * what is !!NOT!! LESS THAN  `T{1} << BIT_SHIFT`
   *
   * \ref test_PLUS_VAL
   */
  template <typename T, T BIT_SHIFT>
  void test_with_plus_expect_failure()
  {
    test<T, BIT_SHIFT, (T{1} << BIT_SHIFT), false>();
  }
};

TEST_F(LogTest, test_0_to_63)
{
  test<uint64_t, 0, 0>();
  test<uint64_t, 1, 0>();
  test<uint64_t, 2, 0>();
  test<uint64_t, 3, 0>();
  test<uint64_t, 4, 0>();
  test<uint64_t, 5, 0>();
  test<uint64_t, 6, 0>();
  test<uint64_t, 7, 0>();
  test<uint64_t, 8, 0>();
  test<uint64_t, 9, 0>();
  test<uint64_t, 10, 0>();
  test<uint64_t, 11, 0>();
  test<uint64_t, 12, 0>();
  test<uint64_t, 13, 0>();
  test<uint64_t, 14, 0>();
  test<uint64_t, 15, 0>();
  test<uint64_t, 16, 0>();
  test<uint64_t, 17, 0>();
  test<uint64_t, 18, 0>();
  test<uint64_t, 19, 0>();
  test<uint64_t, 20, 0>();
  test<uint64_t, 21, 0>();
  test<uint64_t, 22, 0>();
  test<uint64_t, 23, 0>();
  test<uint64_t, 24, 0>();
  test<uint64_t, 25, 0>();
  test<uint64_t, 26, 0>();
  test<uint64_t, 27, 0>();
  test<uint64_t, 28, 0>();
  test<uint64_t, 29, 0>();
  test<uint64_t, 30, 0>();
  test<uint64_t, 31, 0>();
  test<uint64_t, 32, 0>();
  test<uint64_t, 33, 0>();
  test<uint64_t, 34, 0>();
  test<uint64_t, 35, 0>();
  test<uint64_t, 36, 0>();
  test<uint64_t, 37, 0>();
  test<uint64_t, 38, 0>();
  test<uint64_t, 39, 0>();
  test<uint64_t, 40, 0>();
  test<uint64_t, 41, 0>();
  test<uint64_t, 42, 0>();
  test<uint64_t, 43, 0>();
  test<uint64_t, 44, 0>();
  test<uint64_t, 45, 0>();
  test<uint64_t, 46, 0>();
  test<uint64_t, 47, 0>();
  test<uint64_t, 48, 0>();
  test<uint64_t, 49, 0>();
  test<uint64_t, 50, 0>();
  test<uint64_t, 51, 0>();
  test<uint64_t, 52, 0>();
  test<uint64_t, 53, 0>();
  test<uint64_t, 54, 0>();
  test<uint64_t, 55, 0>();
  test<uint64_t, 56, 0>();
  test<uint64_t, 57, 0>();
  test<uint64_t, 58, 0>();
  test<uint64_t, 59, 0>();
  test<uint64_t, 60, 0>();
  test<uint64_t, 61, 0>();
  test<uint64_t, 62, 0>();
  test<uint64_t, 63, 0>();
}

TEST_F(LogTest, test_0_to_63_with_plus)
{
  test_with_plus<uint64_t, 0>();
  test_with_plus<uint64_t, 1>();
  test_with_plus<uint64_t, 2>();
  test_with_plus<uint64_t, 3>();
  test_with_plus<uint64_t, 4>();
  test_with_plus<uint64_t, 5>();
  test_with_plus<uint64_t, 6>();
  test_with_plus<uint64_t, 7>();
  test_with_plus<uint64_t, 8>();
  test_with_plus<uint64_t, 9>();
  test_with_plus<uint64_t, 10>();
  test_with_plus<uint64_t, 11>();
  test_with_plus<uint64_t, 12>();
  test_with_plus<uint64_t, 13>();
  test_with_plus<uint64_t, 14>();
  test_with_plus<uint64_t, 15>();
  test_with_plus<uint64_t, 16>();
  test_with_plus<uint64_t, 17>();
  test_with_plus<uint64_t, 18>();
  test_with_plus<uint64_t, 19>();
  test_with_plus<uint64_t, 20>();
  test_with_plus<uint64_t, 21>();
  test_with_plus<uint64_t, 22>();
  test_with_plus<uint64_t, 23>();
  test_with_plus<uint64_t, 24>();
  test_with_plus<uint64_t, 25>();
  test_with_plus<uint64_t, 26>();
  test_with_plus<uint64_t, 27>();
  test_with_plus<uint64_t, 28>();
  test_with_plus<uint64_t, 29>();
  test_with_plus<uint64_t, 30>();
  test_with_plus<uint64_t, 31>();
  test_with_plus<uint64_t, 32>();
  test_with_plus<uint64_t, 33>();
  test_with_plus<uint64_t, 34>();
  test_with_plus<uint64_t, 35>();
  test_with_plus<uint64_t, 36>();
  test_with_plus<uint64_t, 37>();
  test_with_plus<uint64_t, 38>();
  test_with_plus<uint64_t, 39>();
  test_with_plus<uint64_t, 40>();
  test_with_plus<uint64_t, 41>();
  test_with_plus<uint64_t, 42>();
  test_with_plus<uint64_t, 43>();
  test_with_plus<uint64_t, 44>();
  test_with_plus<uint64_t, 45>();
  test_with_plus<uint64_t, 46>();
  test_with_plus<uint64_t, 47>();
  test_with_plus<uint64_t, 48>();
  test_with_plus<uint64_t, 49>();
  test_with_plus<uint64_t, 50>();
  test_with_plus<uint64_t, 51>();
  test_with_plus<uint64_t, 52>();
  test_with_plus<uint64_t, 53>();
  test_with_plus<uint64_t, 54>();
  test_with_plus<uint64_t, 55>();
  test_with_plus<uint64_t, 56>();
  test_with_plus<uint64_t, 57>();
  test_with_plus<uint64_t, 58>();
  test_with_plus<uint64_t, 59>();
  test_with_plus<uint64_t, 60>();
  test_with_plus<uint64_t, 61>();
  test_with_plus<uint64_t, 62>();
  test_with_plus<uint64_t, 63>();
}

TEST_F(LogTest, test_0_to_63_expected_failure)
{
  test_with_plus_expect_failure<uint64_t, 0>();
  test_with_plus_expect_failure<uint64_t, 1>();
  test_with_plus_expect_failure<uint64_t, 2>();
  test_with_plus_expect_failure<uint64_t, 3>();
  test_with_plus_expect_failure<uint64_t, 4>();
  test_with_plus_expect_failure<uint64_t, 5>();
  test_with_plus_expect_failure<uint64_t, 6>();
  test_with_plus_expect_failure<uint64_t, 7>();
  test_with_plus_expect_failure<uint64_t, 8>();
  test_with_plus_expect_failure<uint64_t, 9>();
  test_with_plus_expect_failure<uint64_t, 10>();
  test_with_plus_expect_failure<uint64_t, 11>();
  test_with_plus_expect_failure<uint64_t, 12>();
  test_with_plus_expect_failure<uint64_t, 13>();
  test_with_plus_expect_failure<uint64_t, 14>();
  test_with_plus_expect_failure<uint64_t, 15>();
  test_with_plus_expect_failure<uint64_t, 16>();
  test_with_plus_expect_failure<uint64_t, 17>();
  test_with_plus_expect_failure<uint64_t, 18>();
  test_with_plus_expect_failure<uint64_t, 19>();
  test_with_plus_expect_failure<uint64_t, 20>();
  test_with_plus_expect_failure<uint64_t, 21>();
  test_with_plus_expect_failure<uint64_t, 22>();
  test_with_plus_expect_failure<uint64_t, 23>();
  test_with_plus_expect_failure<uint64_t, 24>();
  test_with_plus_expect_failure<uint64_t, 25>();
  test_with_plus_expect_failure<uint64_t, 26>();
  test_with_plus_expect_failure<uint64_t, 27>();
  test_with_plus_expect_failure<uint64_t, 28>();
  test_with_plus_expect_failure<uint64_t, 29>();
  test_with_plus_expect_failure<uint64_t, 30>();
  test_with_plus_expect_failure<uint64_t, 31>();
  test_with_plus_expect_failure<uint64_t, 32>();
  test_with_plus_expect_failure<uint64_t, 33>();
  test_with_plus_expect_failure<uint64_t, 34>();
  test_with_plus_expect_failure<uint64_t, 35>();
  test_with_plus_expect_failure<uint64_t, 36>();
  test_with_plus_expect_failure<uint64_t, 37>();
  test_with_plus_expect_failure<uint64_t, 38>();
  test_with_plus_expect_failure<uint64_t, 39>();
  test_with_plus_expect_failure<uint64_t, 40>();
  test_with_plus_expect_failure<uint64_t, 41>();
  test_with_plus_expect_failure<uint64_t, 42>();
  test_with_plus_expect_failure<uint64_t, 43>();
  test_with_plus_expect_failure<uint64_t, 44>();
  test_with_plus_expect_failure<uint64_t, 45>();
  test_with_plus_expect_failure<uint64_t, 46>();
  test_with_plus_expect_failure<uint64_t, 47>();
  test_with_plus_expect_failure<uint64_t, 48>();
  test_with_plus_expect_failure<uint64_t, 49>();
  test_with_plus_expect_failure<uint64_t, 50>();
  test_with_plus_expect_failure<uint64_t, 51>();
  test_with_plus_expect_failure<uint64_t, 52>();
  test_with_plus_expect_failure<uint64_t, 53>();
  test_with_plus_expect_failure<uint64_t, 54>();
  test_with_plus_expect_failure<uint64_t, 55>();
  test_with_plus_expect_failure<uint64_t, 56>();
  test_with_plus_expect_failure<uint64_t, 57>();
  test_with_plus_expect_failure<uint64_t, 58>();
  test_with_plus_expect_failure<uint64_t, 59>();
  test_with_plus_expect_failure<uint64_t, 60>();
  test_with_plus_expect_failure<uint64_t, 61>();
  test_with_plus_expect_failure<uint64_t, 62>();
  test_with_plus_expect_failure<uint64_t, 63>();
}

}  // namespace
}  // namespace meta
}  // namespace fetch