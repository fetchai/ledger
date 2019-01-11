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

#include "core/byte_array/byte_array.hpp"

#include "gtest/gtest.h"
//#include "gmock/gmock.h"

#include <iostream>

namespace fetch {
namespace byte_array {

namespace {

class ByteArrayTest : public testing::Test
{
protected:
  void SetUp()
  {}

  void TearDown()
  {}
};

TEST_F(ByteArrayTest, test_replace)
{
  ByteArray arr("hello kitty, how are you?");

  //* PRODUCTION CODE UNDER TEST
  std::size_t const num_of_replacements = arr.Replace(' ', '-');

  //* EXPECTARTIONS
  ConstByteArray const expected_result("hello-kitty,-how-are-you?");
  EXPECT_EQ(expected_result, arr);
  EXPECT_EQ(4, num_of_replacements);
}

}  // namespace

}  // namespace byte_array
}  // namespace fetch
