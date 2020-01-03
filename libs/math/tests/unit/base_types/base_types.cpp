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

#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

namespace {

template <typename T>
T TypeConstructor(std::string const &val)
{
  return fetch::math::Type<T>(val);
}

template <typename T>
void TestEquivalence(std::string const &val, T expected)
{
  EXPECT_EQ(TypeConstructor<T>(val), expected);
}

template <typename T>
void TestNear(std::string const &val, T expected, T tolerance)
{
  EXPECT_NEAR(static_cast<double>(TypeConstructor<T>(val)), static_cast<double>(expected),
              static_cast<double>(tolerance));
}

template <typename T>
void TestThrow(std::string const &val)
{
  EXPECT_ANY_THROW(TypeConstructor<T>(val));
}

template <typename T>
class TypeConstructionTest : public ::testing::Test
{
};

TEST(TypeConstructionTest, one_construction)
{
  // integers
  TestEquivalence<std::int8_t>("1", std::int8_t(1));
  TestEquivalence<std::int16_t>("1", std::int16_t(1));
  TestEquivalence<std::int32_t>("1", std::int32_t(1));
  TestEquivalence<std::int64_t>("1", std::int64_t(1));

  // unsigned integers
  TestEquivalence<std::uint8_t>("1", std::uint8_t(1));
  TestEquivalence<std::uint16_t>("1", std::uint16_t(1));
  TestEquivalence<std::uint32_t>("1", std::uint32_t(1));
  TestEquivalence<std::uint64_t>("1", std::uint64_t(1));

  // floating
  TestEquivalence<float>("1", float(1));
  TestEquivalence<double>("1", double(1));

  // fixed
  TestEquivalence<fetch::fixed_point::fp32_t>("1", fetch::fixed_point::fp32_t(1));
  TestEquivalence<fetch::fixed_point::fp64_t>("1", fetch::fixed_point::fp64_t(1));
  TestEquivalence<fetch::fixed_point::fp128_t>("1", fetch::fixed_point::fp128_t(1));
}

TEST(TypeConstructionTest, min_one_construction)
{
  // integers
  TestEquivalence<std::int8_t>("-1", std::int8_t(-1));
  TestEquivalence<std::int16_t>("-1", std::int16_t(-1));
  TestEquivalence<std::int32_t>("-1", std::int32_t(-1));
  TestEquivalence<std::int64_t>("-1", std::int64_t(-1));

  // unsigned integers
  TestThrow<std::uint8_t>("-1");
  TestThrow<std::uint16_t>("-1");
  TestThrow<std::uint32_t>("-1");
  TestThrow<std::uint64_t>("-1");

  // floating
  TestEquivalence<float>("-1", float(-1));
  TestEquivalence<double>("-1", double(-1));

  // fixed
  TestEquivalence<fetch::fixed_point::fp32_t>("-1", fetch::fixed_point::fp32_t(-1));
  TestEquivalence<fetch::fixed_point::fp64_t>("-1", fetch::fixed_point::fp64_t(-1));
  TestEquivalence<fetch::fixed_point::fp128_t>("-1", fetch::fixed_point::fp128_t(-1));
}

TEST(TypeConstructionTest, one_point_zero_construction)
{
  // integers
  TestEquivalence<std::int8_t>("1.0", std::int8_t(1.0));
  TestEquivalence<std::int16_t>("1.0", std::int16_t(1.0));
  TestEquivalence<std::int32_t>("1.0", std::int32_t(1.0));
  TestEquivalence<std::int64_t>("1.0", std::int64_t(1.0));

  // unsigned integers
  TestEquivalence<std::uint8_t>("1.0", std::uint8_t(1.0));
  TestEquivalence<std::uint16_t>("1.0", std::uint16_t(1.0));
  TestEquivalence<std::uint32_t>("1.0", std::uint32_t(1.0));
  TestEquivalence<std::uint64_t>("1.0", std::uint64_t(1.0));

  // floating
  TestEquivalence<float>("1.0", float(1.0));
  TestEquivalence<double>("1.0", double(1.0));

  // fixed
  TestEquivalence<fetch::fixed_point::fp32_t>("1.0",
                                              fetch::math::AsType<fetch::fixed_point::fp32_t>(1.0));
  TestEquivalence<fetch::fixed_point::fp64_t>("1.0",
                                              fetch::math::AsType<fetch::fixed_point::fp64_t>(1.0));
  TestEquivalence<fetch::fixed_point::fp128_t>(
      "1.0", fetch::math::AsType<fetch::fixed_point::fp128_t>(1.0));
}

TEST(TypeConstructionTest, min_one_point_zero_construction)
{
  // integers
  TestEquivalence<std::int8_t>("-1.0", std::int8_t(-1.0));
  TestEquivalence<std::int16_t>("-1.0", std::int16_t(-1.0));
  TestEquivalence<std::int32_t>("-1.0", std::int32_t(-1.0));
  TestEquivalence<std::int64_t>("-1.0", std::int64_t(-1.0));

  // unsigned integers
  TestThrow<std::uint8_t>("-1.0");
  TestThrow<std::uint16_t>("-1.0");
  TestThrow<std::uint32_t>("-1.0");
  TestThrow<std::uint64_t>("-1.0");

  // floating
  TestEquivalence<float>("-1.0", float(-1.0));
  TestEquivalence<double>("-1.0", double(-1.0));

  // fixed
  TestEquivalence<fetch::fixed_point::fp32_t>(
      "-1.0", fetch::math::AsType<fetch::fixed_point::fp32_t>(-1.0));
  TestEquivalence<fetch::fixed_point::fp64_t>(
      "-1.0", fetch::math::AsType<fetch::fixed_point::fp64_t>(-1.0));
  TestEquivalence<fetch::fixed_point::fp128_t>(
      "-1.0", fetch::math::AsType<fetch::fixed_point::fp128_t>(-1.0));
}

TEST(TypeConstructionTest, rounding_construction)
{
  // integers
  TestEquivalence<std::int8_t>("-1.123456789", std::int8_t(-1.123456789));
  TestEquivalence<std::int16_t>("-1.123456789", std::int16_t(-1.123456789));
  TestEquivalence<std::int32_t>("-1.123456789", std::int32_t(-1.123456789));
  TestEquivalence<std::int64_t>("-1.123456789", std::int64_t(-1.123456789));

  // unsigned integers
  TestThrow<std::uint8_t>("-1.5");
  TestThrow<std::uint16_t>("-1.5");
  TestThrow<std::uint32_t>("-1.5");
  TestThrow<std::uint64_t>("-1.5");

  // floating
  TestEquivalence<float>("-1.123456789", float(-1.123456789));
  TestEquivalence<double>("-1.123456789", double(-1.123456789));

  // fixed
  TestNear<fetch::fixed_point::fp32_t>(
      "-1.123456789", fetch::math::AsType<fetch::fixed_point::fp32_t>(-1.123456789),
      fetch::math::function_tolerance<fetch::fixed_point::fp32_t>());
  TestNear<fetch::fixed_point::fp64_t>(
      "-1.123456789", fetch::math::AsType<fetch::fixed_point::fp64_t>(-1.123456789),
      fetch::math::function_tolerance<fetch::fixed_point::fp64_t>());
  TestNear<fetch::fixed_point::fp128_t>(
      "-1.123456789", fetch::math::AsType<fetch::fixed_point::fp128_t>(-1.123456789),
      fetch::math::function_tolerance<fetch::fixed_point::fp128_t>());
}

}  // namespace
