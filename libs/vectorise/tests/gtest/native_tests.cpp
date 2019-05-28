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

#include "core/random/lfg.hpp"
#include "vectorise/register.hpp"

#include <gtest/gtest.h>
using namespace fetch::vectorize;

template <typename T>
using NativeRegister = VectorRegister<T>;

fetch::random::LinearCongruentialGenerator lcg;

#define ADD_TEST(OP, NAME)                                                      \
  template <typename T, bool integral = true>                                   \
  void test_##NAME()                                                            \
  {                                                                             \
    T a;                                                                        \
    T b;                                                                        \
    if (integral)                                                               \
    {                                                                           \
      a = T(lcg());                                                             \
      b = T(lcg());                                                             \
    }                                                                           \
    else                                                                        \
    {                                                                           \
      a = T(lcg.AsDouble());                                                    \
      b = T(lcg.AsDouble());                                                    \
    }                                                                           \
    NativeRegister<T> A(a), B(b);                                               \
    NativeRegister<T> C          = A OP B;                                      \
    T                          c = T(a OP b);                                   \
    ASSERT_EQ(T(C), c) << T(C) << " != " << c << "for " #NAME << " using " #OP; \
  }

// clang-format off
ADD_TEST(*, multiply)
ADD_TEST(+, add)
ADD_TEST(-, subtract)
ADD_TEST(/, divide)
ADD_TEST(&, and)
ADD_TEST(|, or)
ADD_TEST(^, xor)
#undef ADD_TEST // NOLINT
// clang-format on

void test_registers()
{
  for (std::size_t i = 0; i < 10000000; ++i)
  {
    test_multiply<int8_t>();
    test_multiply<int16_t>();
    test_multiply<int32_t>();
    test_multiply<int64_t>();

    test_multiply<uint8_t>();
    test_multiply<uint16_t>();
    test_multiply<uint32_t>();
    test_multiply<uint64_t>();

    test_multiply<double, false>();
    test_multiply<float, false>();
    test_multiply<double>();
    test_multiply<float>();

    test_add<int8_t>();
    test_add<int16_t>();
    test_add<int32_t>();
    test_add<int64_t>();

    test_add<uint8_t>();
    test_add<uint16_t>();
    test_add<uint32_t>();
    test_add<uint64_t>();

    test_add<double, false>();
    test_add<float, false>();
    test_add<double>();
    test_add<float>();

    test_subtract<int8_t>();
    test_subtract<int16_t>();
    test_subtract<int32_t>();
    test_subtract<int64_t>();

    test_subtract<uint8_t>();
    test_subtract<uint16_t>();
    test_subtract<uint32_t>();
    test_subtract<uint64_t>();

    test_subtract<double, false>();
    test_subtract<float, false>();
    test_subtract<double>();
    test_subtract<float>();

    test_divide<int8_t>();
    test_divide<int16_t>();
    test_divide<int32_t>();
    test_divide<int64_t>();

    test_divide<uint8_t>();
    test_divide<uint16_t>();
    test_divide<uint32_t>();
    test_divide<uint64_t>();

    test_divide<double, false>();
    test_divide<float, false>();
    test_divide<double>();
    test_divide<float>();

    test_and<int8_t>();
    test_and<int16_t>();
    test_and<int32_t>();
    test_and<int64_t>();

    test_and<uint8_t>();
    test_and<uint16_t>();
    test_and<uint32_t>();
    test_and<uint64_t>();

    test_or<int8_t>();
    test_or<int16_t>();
    test_or<int32_t>();
    test_or<int64_t>();

    test_or<uint8_t>();
    test_or<uint16_t>();
    test_or<uint32_t>();
    test_or<uint64_t>();

    test_xor<int8_t>();
    test_xor<int16_t>();
    test_xor<int32_t>();
    test_xor<int64_t>();

    test_xor<uint8_t>();
    test_xor<uint16_t>();
    test_xor<uint32_t>();
    test_xor<uint64_t>();
  }
}

#define ADD_TEST(OP, NAME)                                                      \
  template <typename T, bool integral = true>                                   \
  void mtest_##NAME()                                                           \
  {                                                                             \
    T a;                                                                        \
    T b;                                                                        \
    if (integral)                                                               \
    {                                                                           \
      a = lcg();                                                                \
      b = lcg();                                                                \
    }                                                                           \
    else                                                                        \
    {                                                                           \
      a = lcg.AsDouble();                                                       \
      b = lcg.AsDouble();                                                       \
    }                                                                           \
    NativeRegister<T> A(a), B(b);                                               \
    NativeRegister<T> C          = A OP B;                                      \
    T                          c = T(a OP b);                                   \
    ASSERT_EQ(T(C), c) << T(C) << " != " << c << "for " #NAME << " using " #OP; \
  }

// clang-format off
ADD_TEST(*, multiply)
ADD_TEST(+, add)
ADD_TEST(-, subtract)
ADD_TEST(/, divide)
ADD_TEST(&, and)
ADD_TEST(|, or)
ADD_TEST (^, xor) // NOLINT
// clang-format on

#undef ADD_TEST

TEST(vectorise_native_test, test_registers)
{
  test_registers();
}
