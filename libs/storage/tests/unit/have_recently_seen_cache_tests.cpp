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
#include "storage/have_seen_recently_cache.hpp"

using namespace fetch::storage;

// Test the normal functionality of the cache
TEST(HaveSeenRecentlyCache, basic_functionality)
{
  uint64_t                           cache_size = 3;
  HaveSeenRecentlyCache<std::string> cache{cache_size};

  cache.Add("A");
  cache.Add("B");
  cache.Add("C");

  EXPECT_TRUE(cache.Seen("A"));
  EXPECT_TRUE(cache.Seen("B"));
  EXPECT_TRUE(cache.Seen("C"));
  EXPECT_FALSE(cache.Seen("D"));
  EXPECT_FALSE(cache.Seen("a"));
  EXPECT_FALSE(cache.Seen(""));
}

// Test the cache still works with a size of 0
TEST(HaveSeenRecentlyCache, zero_cache)
{
  uint64_t                           cache_size = 0;
  HaveSeenRecentlyCache<std::string> cache{cache_size};

  cache.Add("A");
  cache.Add("B");
  cache.Add("C");

  EXPECT_FALSE(cache.Seen("A"));
  EXPECT_FALSE(cache.Seen("B"));
  EXPECT_FALSE(cache.Seen("C"));
}

// Test that when more elements are put in than the cache limit it will
// indicate it has not seen
TEST(HaveSeenRecentlyCache, cache_size_limit)
{
  uint64_t                           cache_size = 3;
  HaveSeenRecentlyCache<std::string> cache{cache_size};

  cache.Add("A");
  cache.Add("B");
  cache.Add("C");
  cache.Add("D");

  EXPECT_FALSE(cache.Seen("A"));
  EXPECT_TRUE(cache.Seen("B"));
  EXPECT_TRUE(cache.Seen("C"));
  EXPECT_TRUE(cache.Seen("D"));
}
