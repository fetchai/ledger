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
  std::cout << "val: " << val << std::endl;
  std::cout << "expected: " << expected << std::endl;
  EXPECT_EQ(TypeConstructor<T>(val), expected);
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
  TestEquivalence<std::uint8_t>("-1", std::uint8_t(-1));
  TestEquivalence<std::uint16_t>("-1", std::uint16_t(-1));
  TestEquivalence<std::uint32_t>("-1", std::uint32_t(-1));
  TestEquivalence<std::uint64_t>("-1", std::uint64_t(-1));

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
  TestEquivalence<fetch::fixed_point::fp32_t>("1.0", fetch::fixed_point::fp32_t(1.0));
  TestEquivalence<fetch::fixed_point::fp64_t>("1.0", fetch::fixed_point::fp64_t(1.0));
  TestEquivalence<fetch::fixed_point::fp128_t>("1.0", fetch::fixed_point::fp128_t(1.0));
}

TEST(TypeConstructionTest, min_one_point_zero_construction)
{
  // integers
  TestEquivalence<std::int8_t>("-1.0", std::int8_t(-1.0));
  TestEquivalence<std::int16_t>("-1.0", std::int16_t(-1.0));
  TestEquivalence<std::int32_t>("-1.0", std::int32_t(-1.0));
  TestEquivalence<std::int64_t>("-1.0", std::int64_t(-1.0));

  // unsigned integers
  TestEquivalence<std::uint8_t>("-1.0", std::uint8_t(-1.0));
  TestEquivalence<std::uint16_t>("-1.0", std::uint16_t(-1.0));
  TestEquivalence<std::uint32_t>("-1.0", std::uint32_t(-1.0));
  TestEquivalence<std::uint64_t>("-1.0", std::uint64_t(-1.0));

  // floating
  TestEquivalence<float>("-1.0", float(-1.0));
  TestEquivalence<double>("-1.0", double(-1.0));

  // fixed
  TestEquivalence<fetch::fixed_point::fp32_t>("-1.0", fetch::fixed_point::fp32_t(-1.0));
  TestEquivalence<fetch::fixed_point::fp64_t>("-1.0", fetch::fixed_point::fp64_t(-1.0));
  TestEquivalence<fetch::fixed_point::fp128_t>("-1.0", fetch::fixed_point::fp128_t(-1.0));
}

// TODO - test bools and chars negative cases
// test 1.5 for int

}  // namespace
