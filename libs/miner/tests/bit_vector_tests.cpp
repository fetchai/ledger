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

#include "core/byte_array/const_byte_array.hpp"
#include "miner/optimisation/bitvector.hpp"
#include "vectorise/platform.hpp"

#include "gtest/gtest.h"

#include <iostream>
#include <iomanip>
#include <memory>

using fetch::bitmanip::BitVector;
using fetch::platform::IsLog2;
using fetch::platform::ToLog2;
using fetch::platform::DivCeil;

template <typename T>
void BinPrint(T value, char const *name)
{
  constexpr std::size_t BIT_SIZE = sizeof(T) * 8u;
  constexpr std::size_t INDEX    = BIT_SIZE - 1u;

  std::cout << name << ": ";

  for (std::size_t i = 0, j = INDEX; i < BIT_SIZE; ++i, --j)
  {
    if (i && ((i & 0x3) == 0))
    {
      std::cout << ' ';
    }

    if (i && ((i & 0x7) == 0))
    {
      std::cout << "- ";
    }

    if (value & (1ull << j))
    {
      std::cout << '1';
    }
    else
    {
      std::cout << '0';
    }
  }

  std::cout << std::endl;
}

template <typename T>
fetch::byte_array::ConstByteArray Convert(T const &value)
{
  auto const *raw = reinterpret_cast<uint8_t const *>(&value);

  return {raw, sizeof(T)};
}

fetch::byte_array::ConstByteArray Convert(BitVector const &value)
{
  auto const *raw = reinterpret_cast<uint8_t const *>(value.data().pointer());

  return {raw, sizeof(BitVector::data_type) * value.blocks()};
}



TEST(BitVectorTests, ExpandWildcard0)
{
  BitVector wildcard{};

  BitVector small{4};
  ASSERT_TRUE(wildcard.RemapTo(small));

  EXPECT_EQ(small.bit(0), 1);
  EXPECT_EQ(small.bit(1), 1);
  EXPECT_EQ(small.bit(2), 1);
  EXPECT_EQ(small.bit(3), 1);

  BitVector large{16};
  ASSERT_TRUE(wildcard.RemapTo(large));

  EXPECT_EQ(large.bit(0), 1);
  EXPECT_EQ(large.bit(1), 1);
  EXPECT_EQ(large.bit(2), 1);
  EXPECT_EQ(large.bit(3), 1);
  EXPECT_EQ(large.bit(4), 1);
  EXPECT_EQ(large.bit(5), 1);
  EXPECT_EQ(large.bit(6), 1);
  EXPECT_EQ(large.bit(7), 1);
  EXPECT_EQ(large.bit(8), 1);
  EXPECT_EQ(large.bit(9), 1);
  EXPECT_EQ(large.bit(10), 1);
  EXPECT_EQ(large.bit(11), 1);
  EXPECT_EQ(large.bit(12), 1);
  EXPECT_EQ(large.bit(13), 1);
  EXPECT_EQ(large.bit(14), 1);
  EXPECT_EQ(large.bit(15), 1);
}

TEST(BitVectorTests, ExpandWildcard1)
{
  BitVector wildcard{};

  BitVector small{4};
  ASSERT_TRUE(wildcard.RemapTo(small));

  EXPECT_EQ(small.bit(0), 1);
  EXPECT_EQ(small.bit(1), 1);
  EXPECT_EQ(small.bit(2), 1);
  EXPECT_EQ(small.bit(3), 1);

  BitVector large{16};
  ASSERT_TRUE(wildcard.RemapTo(large));

  EXPECT_EQ(large.bit(0), 1);
  EXPECT_EQ(large.bit(1), 1);
  EXPECT_EQ(large.bit(2), 1);
  EXPECT_EQ(large.bit(3), 1);
  EXPECT_EQ(large.bit(4), 1);
  EXPECT_EQ(large.bit(5), 1);
  EXPECT_EQ(large.bit(6), 1);
  EXPECT_EQ(large.bit(7), 1);
  EXPECT_EQ(large.bit(8), 1);
  EXPECT_EQ(large.bit(9), 1);
  EXPECT_EQ(large.bit(10), 1);
  EXPECT_EQ(large.bit(11), 1);
  EXPECT_EQ(large.bit(12), 1);
  EXPECT_EQ(large.bit(13), 1);
  EXPECT_EQ(large.bit(14), 1);
  EXPECT_EQ(large.bit(15), 1);
}

TEST(BitVectorTests, SmallExpand)
{
  BitVector src{2};
  src.set(0, 1);
  src.set(1, 0);

  BitVector bit4{4};
  ASSERT_TRUE(src.RemapTo(bit4));

  EXPECT_EQ(bit4.bit(0), 1);
  EXPECT_EQ(bit4.bit(1), 1);
  EXPECT_EQ(bit4.bit(2), 0);
  EXPECT_EQ(bit4.bit(3), 0);

  BitVector bit8{8};
  ASSERT_TRUE(src.RemapTo(bit8));

  EXPECT_EQ(bit8.bit(0), 1);
  EXPECT_EQ(bit8.bit(1), 1);
  EXPECT_EQ(bit8.bit(2), 1);
  EXPECT_EQ(bit8.bit(3), 1);
  EXPECT_EQ(bit8.bit(4), 0);
  EXPECT_EQ(bit8.bit(5), 0);
  EXPECT_EQ(bit8.bit(6), 0);
  EXPECT_EQ(bit8.bit(7), 0);
}

TEST(BitVectorTests, ExpandTo16)
{
  // set and initial mask
  BitVector mask{8};
  mask.set(0, 1);
  mask.set(3, 1);
  mask.set(7, 1);

  BitVector other{16};
  ASSERT_TRUE(mask.RemapTo(other));

  EXPECT_EQ(other.bit(0), 1);
  EXPECT_EQ(other.bit(1), 1);
  EXPECT_EQ(other.bit(2), 0);
  EXPECT_EQ(other.bit(3), 0);
  EXPECT_EQ(other.bit(4), 0);
  EXPECT_EQ(other.bit(5), 0);
  EXPECT_EQ(other.bit(6), 1);
  EXPECT_EQ(other.bit(7), 1);
  EXPECT_EQ(other.bit(8), 0);
  EXPECT_EQ(other.bit(9), 0);
  EXPECT_EQ(other.bit(10), 0);
  EXPECT_EQ(other.bit(11), 0);
  EXPECT_EQ(other.bit(12), 0);
  EXPECT_EQ(other.bit(13), 0);
  EXPECT_EQ(other.bit(14), 1);
  EXPECT_EQ(other.bit(15), 1);
}

TEST(BitVectorTests, ExpandTo32)
{
  // set and initial mask
  BitVector mask{8};
  mask.set(0, 1);
  mask.set(3, 1);
  mask.set(7, 1);

  BitVector other{32};
  ASSERT_TRUE(mask.RemapTo(other));

  EXPECT_EQ(other.bit(0), 1);
  EXPECT_EQ(other.bit(1), 1);
  EXPECT_EQ(other.bit(2), 1);
  EXPECT_EQ(other.bit(3), 1);
  EXPECT_EQ(other.bit(4), 0);
  EXPECT_EQ(other.bit(5), 0);
  EXPECT_EQ(other.bit(6), 0);
  EXPECT_EQ(other.bit(7), 0);
  EXPECT_EQ(other.bit(8), 0);
  EXPECT_EQ(other.bit(9), 0);
  EXPECT_EQ(other.bit(10), 0);
  EXPECT_EQ(other.bit(11), 0);
  EXPECT_EQ(other.bit(12), 1);
  EXPECT_EQ(other.bit(13), 1);
  EXPECT_EQ(other.bit(14), 1);
  EXPECT_EQ(other.bit(15), 1);
  EXPECT_EQ(other.bit(16), 0);
  EXPECT_EQ(other.bit(17), 0);
  EXPECT_EQ(other.bit(18), 0);
  EXPECT_EQ(other.bit(19), 0);
  EXPECT_EQ(other.bit(20), 0);
  EXPECT_EQ(other.bit(21), 0);
  EXPECT_EQ(other.bit(22), 0);
  EXPECT_EQ(other.bit(23), 0);
  EXPECT_EQ(other.bit(24), 0);
  EXPECT_EQ(other.bit(25), 0);
  EXPECT_EQ(other.bit(26), 0);
  EXPECT_EQ(other.bit(27), 0);
  EXPECT_EQ(other.bit(28), 1);
  EXPECT_EQ(other.bit(29), 1);
  EXPECT_EQ(other.bit(30), 1);
  EXPECT_EQ(other.bit(31), 1);
}

TEST(BitVectorTests, ExpandTo64)
{
  // set and initial mask
  BitVector mask{8};
  mask.set(0, 1);
  mask.set(3, 1);
  mask.set(7, 1);

  BitVector other{64};
  ASSERT_TRUE(mask.RemapTo(other));

  EXPECT_EQ(other.bit(0), 1);
  EXPECT_EQ(other.bit(1), 1);
  EXPECT_EQ(other.bit(2), 1);
  EXPECT_EQ(other.bit(3), 1);
  EXPECT_EQ(other.bit(4), 1);
  EXPECT_EQ(other.bit(5), 1);
  EXPECT_EQ(other.bit(6), 1);
  EXPECT_EQ(other.bit(7), 1);
  EXPECT_EQ(other.bit(8), 0);
  EXPECT_EQ(other.bit(9), 0);
  EXPECT_EQ(other.bit(10), 0);
  EXPECT_EQ(other.bit(11), 0);
  EXPECT_EQ(other.bit(12), 0);
  EXPECT_EQ(other.bit(13), 0);
  EXPECT_EQ(other.bit(14), 0);
  EXPECT_EQ(other.bit(15), 0);
  EXPECT_EQ(other.bit(16), 0);
  EXPECT_EQ(other.bit(17), 0);
  EXPECT_EQ(other.bit(18), 0);
  EXPECT_EQ(other.bit(19), 0);
  EXPECT_EQ(other.bit(20), 0);
  EXPECT_EQ(other.bit(21), 0);
  EXPECT_EQ(other.bit(22), 0);
  EXPECT_EQ(other.bit(23), 0);
  EXPECT_EQ(other.bit(24), 1);
  EXPECT_EQ(other.bit(25), 1);
  EXPECT_EQ(other.bit(26), 1);
  EXPECT_EQ(other.bit(27), 1);
  EXPECT_EQ(other.bit(28), 1);
  EXPECT_EQ(other.bit(29), 1);
  EXPECT_EQ(other.bit(30), 1);
  EXPECT_EQ(other.bit(31), 1);
  EXPECT_EQ(other.bit(32), 0);
  EXPECT_EQ(other.bit(33), 0);
  EXPECT_EQ(other.bit(34), 0);
  EXPECT_EQ(other.bit(35), 0);
  EXPECT_EQ(other.bit(36), 0);
  EXPECT_EQ(other.bit(37), 0);
  EXPECT_EQ(other.bit(38), 0);
  EXPECT_EQ(other.bit(39), 0);
  EXPECT_EQ(other.bit(40), 0);
  EXPECT_EQ(other.bit(41), 0);
  EXPECT_EQ(other.bit(42), 0);
  EXPECT_EQ(other.bit(43), 0);
  EXPECT_EQ(other.bit(44), 0);
  EXPECT_EQ(other.bit(45), 0);
  EXPECT_EQ(other.bit(46), 0);
  EXPECT_EQ(other.bit(47), 0);
  EXPECT_EQ(other.bit(48), 0);
  EXPECT_EQ(other.bit(49), 0);
  EXPECT_EQ(other.bit(50), 0);
  EXPECT_EQ(other.bit(51), 0);
  EXPECT_EQ(other.bit(52), 0);
  EXPECT_EQ(other.bit(53), 0);
  EXPECT_EQ(other.bit(54), 0);
  EXPECT_EQ(other.bit(55), 0);
  EXPECT_EQ(other.bit(56), 1);
  EXPECT_EQ(other.bit(57), 1);
  EXPECT_EQ(other.bit(58), 1);
  EXPECT_EQ(other.bit(59), 1);
  EXPECT_EQ(other.bit(60), 1);
  EXPECT_EQ(other.bit(61), 1);
  EXPECT_EQ(other.bit(62), 1);
  EXPECT_EQ(other.bit(63), 1);
}

TEST(BitVectorTests, ContractFrom8)
{
  // set and initial mask
  BitVector mask{8};
  mask.set(0, 1);
  mask.set(3, 1);
  mask.set(4, 1);
  mask.set(5, 1);

  BitVector mask4{4};
  ASSERT_TRUE(mask.RemapTo(mask4));

  EXPECT_EQ(mask4.bit(0), 1);
  EXPECT_EQ(mask4.bit(1), 1);
  EXPECT_EQ(mask4.bit(2), 1);
  EXPECT_EQ(mask4.bit(3), 0);

  BitVector mask2{2};
  ASSERT_TRUE(mask.RemapTo(mask2));

  EXPECT_EQ(mask2.bit(0), 1);
  EXPECT_EQ(mask2.bit(1), 1);

  BitVector wildcard1{1};
  ASSERT_TRUE(mask.RemapTo(wildcard1));

  EXPECT_EQ(wildcard1.bit(0), 1);

  BitVector wildcard0{};
  ASSERT_TRUE(mask.RemapTo(wildcard0));
}

TEST(BitVectorTests, ContractFrom16)
{
  // set and initial mask
  BitVector mask{16};
  mask.set(1, 1);
  mask.set(7, 1);
  mask.set(9, 1);
  mask.set(10, 1);

  BitVector other{8};
  ASSERT_TRUE(mask.RemapTo(other));

  EXPECT_EQ(other.bit(0), 1);
  EXPECT_EQ(other.bit(1), 0);
  EXPECT_EQ(other.bit(2), 0);
  EXPECT_EQ(other.bit(3), 1);
  EXPECT_EQ(other.bit(4), 1);
  EXPECT_EQ(other.bit(5), 1);
  EXPECT_EQ(other.bit(6), 0);
  EXPECT_EQ(other.bit(7), 0);
}

TEST(BitVectorTests, ContractFrom32)
{
  // set and initial mask
  BitVector mask{32};
  mask.set(0, 1);
  mask.set(3, 1);
  mask.set(6, 1);
  mask.set(7, 1);
  mask.set(9, 1);
  mask.set(10, 1);
  mask.set(12, 1);
  mask.set(13, 1);
  mask.set(20, 1);
  mask.set(21, 1);
  mask.set(22, 1);
  mask.set(23, 1);
  mask.set(25, 1);
  mask.set(27, 1);
  mask.set(28, 1);
  mask.set(30, 1);

  BitVector other{16};
  ASSERT_TRUE(mask.RemapTo(other));

  EXPECT_EQ(other.bit(0), 1);
  EXPECT_EQ(other.bit(1), 1);
  EXPECT_EQ(other.bit(2), 0);
  EXPECT_EQ(other.bit(3), 1);
  EXPECT_EQ(other.bit(4), 1);
  EXPECT_EQ(other.bit(5), 1);
  EXPECT_EQ(other.bit(6), 1);
  EXPECT_EQ(other.bit(7), 0);
  EXPECT_EQ(other.bit(8), 0);
  EXPECT_EQ(other.bit(9), 0);
  EXPECT_EQ(other.bit(10), 1);
  EXPECT_EQ(other.bit(11), 1);
  EXPECT_EQ(other.bit(12), 1);
  EXPECT_EQ(other.bit(13), 1);
  EXPECT_EQ(other.bit(14), 1);
  EXPECT_EQ(other.bit(15), 1);

  BitVector smaller{8};
  ASSERT_TRUE(mask.RemapTo(smaller));

  EXPECT_EQ(smaller.bit(0), 1);
  EXPECT_EQ(smaller.bit(1), 1);
  EXPECT_EQ(smaller.bit(2), 1);
  EXPECT_EQ(smaller.bit(3), 1);
  EXPECT_EQ(smaller.bit(4), 0);
  EXPECT_EQ(smaller.bit(5), 1);
  EXPECT_EQ(smaller.bit(6), 1);
  EXPECT_EQ(smaller.bit(7), 1);
}
