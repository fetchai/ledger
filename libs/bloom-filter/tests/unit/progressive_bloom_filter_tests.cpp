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

#include "bloom_filter/progressive_bloom_filter.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include "gmock/gmock.h"

#include <cstddef>

namespace {

using namespace fetch;

class ProgressiveBloomFilterTests : public ::testing::Test
{
public:
  uint64_t const         overlap{100};
  ProgressiveBloomFilter filter{overlap};
};

TEST_F(ProgressiveBloomFilterTests, match_elements_that_had_been_added)
{
  filter.Add("a", 10, 1);

  ASSERT_TRUE(filter.Match("a", 10).first);
}

TEST_F(ProgressiveBloomFilterTests, do_not_match_elements_that_had_not_been_added)
{
  filter.Add("a", 10, 1);

  ASSERT_FALSE(filter.Match("b", 5).first);
}

TEST_F(ProgressiveBloomFilterTests, filter_rolls_over_in_steps_of_overlap)
{
  filter.Add("a", 10, 1);
  filter.Add("b", 110, overlap + 1);
  filter.Add("c", 240, 2 * overlap + 1);

  ASSERT_FALSE(filter.Match("a", 10).first);
  ASSERT_TRUE(filter.Match("b", 110).first);
  ASSERT_TRUE(filter.Match("c", 240).first);

  filter.Add("d", 340, 3 * overlap + 1);

  ASSERT_FALSE(filter.Match("a", 10).first);
  ASSERT_FALSE(filter.Match("b", 110).first);
  ASSERT_TRUE(filter.Match("c", 240).first);
  ASSERT_TRUE(filter.Match("d", 340).first);
}

TEST_F(ProgressiveBloomFilterTests, do_not_add_elements_older_than_current_head)
{
  filter.Add("a", 240, 2 * overlap + 1);
  filter.Add("b", 199, 2 * overlap + 1);

  ASSERT_TRUE(filter.Match("a", 240).first);
  ASSERT_FALSE(filter.Match("b", 20).first);
}

TEST_F(ProgressiveBloomFilterTests, do_not_add_elements_which_are_newer_than_double_the_overlap)
{
  filter.Add("a", 250, 1);

  ASSERT_FALSE(filter.Match("a", 250).first);
}

}  // namespace
