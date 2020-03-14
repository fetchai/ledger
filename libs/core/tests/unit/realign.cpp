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

#include "core/realign.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstring>

namespace {

using fetch::memory::Realign;

TEST(RealignTests, WhenBufferAligned)
{
  static constexpr std::size_t nints           = 5;
  auto                         ints            = new int[nints];
  char *                       aligned_storage = reinterpret_cast<char *>(ints);

  ASSERT_EQ(reinterpret_cast<char *>(Realign<int>(aligned_storage, nints)), aligned_storage);

  delete[] ints;
}

TEST(RealignTests, WhenBufferMisaligned)
{
  int                          pattern[]    = {42, 31416, 0xDEAD, 0xBEEF};
  static constexpr std::size_t pattern_size = sizeof(pattern) / sizeof(pattern[0]);
  static constexpr std::size_t intalign     = alignof(int);

  auto  ints            = new int[5];
  char *aligned_storage = reinterpret_cast<char *>(ints);

  assert(intalign > 1);
  char *misaligned_storage = aligned_storage + intalign / 2;

  std::memcpy(misaligned_storage, pattern, sizeof(pattern));
  auto deserialized_pattern = Realign<int>(misaligned_storage, pattern_size);

  ASSERT_NE(reinterpret_cast<char *>(deserialized_pattern), misaligned_storage);
  ASSERT_NE(reinterpret_cast<char *>(deserialized_pattern), aligned_storage);

  ASSERT_TRUE(std::equal(pattern, pattern + pattern_size, deserialized_pattern));

  delete[] ints;
  delete[] deserialized_pattern;
}

}  // namespace
