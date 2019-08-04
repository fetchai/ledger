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

#include "core/containers/array.hpp"

#include "gtest/gtest.h"

#include <array>

namespace {

using namespace fetch::core;

class ArrayTests : public ::testing::Test
{
protected:
  using ArrU64 = Array<uint64_t, 4>;
};

TEST_F(ArrayTests, test_default_constr)
{
  using Memory = uint64_t[10];
  static_assert(sizeof(Memory) >= ArrU64::size() * sizeof(ArrU64::value_type),
                "Size of pre-set memory must be bigger or equal to memory needed for array type "
                "under the test.");

  Memory mem = {0x0f0f0f0f0f0f0f0full, 0xf0f0f0f0f0f0f0f0ull, 0x0707070707070707ull,
                0x7070707070707070ull, 0x0a0a0a0a0a0a0a0aull, 0xa0a0a0a0a0a0a0a0ull,
                0x0505050505050505ull, 0x5050505050505050ull, 0xaaaaaaaaaaaaaaaaull,
                0x5555555555555555ull};

  std::size_t i{0};
  for (auto const &itm : mem)
  {
    ASSERT_NE(itm, 0ll);
    ++i;
  }
  ASSERT_EQ(sizeof(Memory) / sizeof(mem[0]), i);

  auto const &arr = *new (mem) ArrU64{};
  for (auto const &itm : arr)
  {
    EXPECT_EQ(itm, 0ll);
  }
}

TEST_F(ArrayTests, test_aggregate_initialisation)
{
  uint64_t mem[] = {0x0f0f0f0f0f0f0f0full, 0xf0f0f0f0f0f0f0f0ull, 0x0707070707070707ull,
                    0x7070707070707070ull, 0x0a0a0a0a0a0a0a0aull, 0xa0a0a0a0a0a0a0a0ull,
                    0x0505050505050505ull, 0x5050505050505050ull, 0xaaaaaaaaaaaaaaaaull,
                    0x5555555555555555ull};
  static_assert(sizeof(mem) >= ArrU64::size() * sizeof(ArrU64::value_type),
                "Size of pre-set memory must be bigger or equal to memory needed for array type "
                "under the test.");

  auto const &arr = *new (mem) ArrU64{1ull, 2ull};
  EXPECT_EQ(1ull, arr[0]);
  EXPECT_EQ(2ull, arr[1]);
  EXPECT_EQ(0ull, arr[2]);
  EXPECT_EQ(0ull, arr[3]);

  ASSERT_EQ(0x0a0a0a0a0a0a0a0aull, mem[4]);
}

TEST_F(ArrayTests, test_forward_iteration_1)
{
  uint64_t mem[] = {0x0f0f0f0f0f0f0f0full, 0xf0f0f0f0f0f0f0f0ull, 0x0707070707070707ull,
                    0x7070707070707070ull, 0x0a0a0a0a0a0a0a0aull, 0xa0a0a0a0a0a0a0a0ull,
                    0x0505050505050505ull, 0x5050505050505050ull, 0xaaaaaaaaaaaaaaaaull,
                    0x5555555555555555ull};
  static_assert(sizeof(mem) >= ArrU64::size() * sizeof(ArrU64::value_type),
                "Size of pre-set memory must be bigger or equal to memory needed for array type "
                "under the test.");

  auto const &arr = *new (mem) ArrU64{1ull, 2ull};

  uint64_t const exp_order[] = {1ull, 2ull, 0ull, 0ull};
  std::size_t    i{0};
  for (auto it{arr.cbegin()}; it != arr.cend(); ++it)
  {
    ASSERT_LT(i, sizeof(exp_order) / sizeof(uint64_t));
    EXPECT_EQ(exp_order[i], *it);
    ++i;
  }
}

TEST_F(ArrayTests, test_reverse_iteration)
{
  uint64_t mem[] = {0x0f0f0f0f0f0f0f0full, 0xf0f0f0f0f0f0f0f0ull, 0x0707070707070707ull,
                    0x7070707070707070ull, 0x0a0a0a0a0a0a0a0aull, 0xa0a0a0a0a0a0a0a0ull,
                    0x0505050505050505ull, 0x5050505050505050ull, 0xaaaaaaaaaaaaaaaaull,
                    0x5555555555555555ull};
  static_assert(sizeof(mem) >= ArrU64::size() * sizeof(ArrU64::value_type),
                "Size of pre-set memoty must be bigger or equal to memory needed for array type "
                "under the test.");

  auto const &arr = *new (mem) ArrU64{1ull, 2ull};

  uint64_t const exp_order[] = {0ull, 0ull, 2ull, 1ull};
  std::size_t    i{0};
  for (auto rit{arr.crbegin()}; rit != arr.crend(); ++rit)
  {
    ASSERT_LT(i, sizeof(exp_order) / sizeof(uint64_t));
    EXPECT_EQ(exp_order[i], *rit);
    ++i;
  }
}

TEST_F(ArrayTests, test_reverse_iteration_with_assignment)
{
  uint64_t mem[] = {0x0f0f0f0f0f0f0f0full, 0xf0f0f0f0f0f0f0f0ull, 0x0707070707070707ull,
                    0x7070707070707070ull, 0x0a0a0a0a0a0a0a0aull, 0xa0a0a0a0a0a0a0a0ull,
                    0x0505050505050505ull, 0x5050505050505050ull, 0xaaaaaaaaaaaaaaaaull,
                    0x5555555555555555ull};
  static_assert(sizeof(mem) >= ArrU64::size() * sizeof(ArrU64::value_type),
                "Size of pre-set memoty must be bigger or equal to memory needed for array type "
                "under the test.");

  auto &arr = *new (mem) ArrU64{};

  uint64_t const exp_order[] = {1ull, 2ull, 0ull, 0ull};
  std::size_t    i{0};
  for (auto rit{arr.rbegin()}; rit != arr.rend(); ++rit)
  {
    ASSERT_LT(i, sizeof(exp_order) / sizeof(uint64_t));
    *rit = exp_order[i];
    EXPECT_EQ(exp_order[i], arr[arr.size() - 1 - i]);
    ++i;
  }
}

TEST_F(ArrayTests, test_forward_iteration_with_assignment)
{
  uint64_t mem[] = {0x0f0f0f0f0f0f0f0full, 0xf0f0f0f0f0f0f0f0ull, 0x0707070707070707ull,
                    0x7070707070707070ull, 0x0a0a0a0a0a0a0a0aull, 0xa0a0a0a0a0a0a0a0ull,
                    0x0505050505050505ull, 0x5050505050505050ull, 0xaaaaaaaaaaaaaaaaull,
                    0x5555555555555555ull};
  static_assert(sizeof(mem) >= ArrU64::size() * sizeof(ArrU64::value_type),
                "Size of pre-set memoty must be bigger or equal to memory needed for array type "
                "under the test.");

  auto &arr = *new (mem) ArrU64{};

  uint64_t const exp_order[] = {1ull, 2ull, 0ull, 0ull};
  std::size_t    i{0};
  for (auto it{arr.begin()}; it != arr.end(); ++it)
  {
    ASSERT_LT(i, sizeof(exp_order) / sizeof(uint64_t));
    *it = exp_order[i];
    EXPECT_EQ(exp_order[i], arr[i]);
    ++i;
  }
}

TEST_F(ArrayTests, test_reverse_begin_end_if_empty)
{
  Array<uint64_t, 0> const arr;
  EXPECT_EQ(arr.crbegin(), arr.crend());
}

TEST_F(ArrayTests, test_forward_begin_end_if_empty)
{
  Array<uint64_t, 0> const arr;
  EXPECT_EQ(arr.cbegin(), arr.cend());
}

}  // namespace
