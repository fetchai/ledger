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

#include "bloom_filter/historical_bloom_filter.hpp"

#include "gtest/gtest.h"

namespace {

using fetch::bloom::HistoricalBloomFilter;

constexpr uint64_t    WINDOW_SIZE = 10;
constexpr std::size_t MAX_CACHED  = 1;

class HistoricalBloomFilterTests : public ::testing::Test
{
protected:

  HistoricalBloomFilter bloom_{HistoricalBloomFilter::Mode::NEW_DATABASE,
                               "h-bloom-tests.db",
                               "h-bloom-tests.index.db",
                               "h-bloom-tests.meta.db",
                               WINDOW_SIZE,
                               MAX_CACHED};
};

TEST_F(HistoricalBloomFilterTests, BasicCheck)
{
  ASSERT_TRUE(bloom_.Add("A", 1));
  ASSERT_TRUE(bloom_.Add("B", 2));
  ASSERT_TRUE(bloom_.Add("C", 3));

  ASSERT_TRUE(bloom_.Match("A", 1, 1));
  ASSERT_TRUE(bloom_.Match("B", 2, 2));
  ASSERT_TRUE(bloom_.Match("C", 3, 3));
}

TEST_F(HistoricalBloomFilterTests, CheckSimpleBucketLookup)
{
  ASSERT_TRUE(bloom_.Add("A", 3));

  ASSERT_TRUE(bloom_.Match("A", 1, 5));
}

TEST_F(HistoricalBloomFilterTests, CheckWindowedLookup)
{
  ASSERT_TRUE(bloom_.Add("A", 30));

  ASSERT_TRUE(bloom_.Match("A", 10, 50));
}

TEST_F(HistoricalBloomFilterTests, CheckWindowedFailLookup)
{
  ASSERT_TRUE(bloom_.Add("A", 30));

  ASSERT_FALSE(bloom_.Match("A", 10, 20));
}

TEST_F(HistoricalBloomFilterTests, CheckFalseMatch)
{
  ASSERT_FALSE(bloom_.Match("A", 1, 2));
}

TEST_F(HistoricalBloomFilterTests, CheckBasicCacheTrimming)
{
  bloom_.Add("A", 1);
  bloom_.Add("B", 20);

  ASSERT_EQ(1, bloom_.TrimCache());
}

TEST_F(HistoricalBloomFilterTests, CheckUpdatesToFlushedPage)
{
  bloom_.Add("A", 1);
  bloom_.Add("B", 20);

  ASSERT_EQ(1, bloom_.TrimCache());

  bloom_.Add("C", 2); // <- update to flushed page

  ASSERT_TRUE(bloom_.Match("A", 1, 20));
  ASSERT_TRUE(bloom_.Match("B", 1, 20));
  ASSERT_TRUE(bloom_.Match("C", 1, 20));
}

TEST_F(HistoricalBloomFilterTests, CheckUpdatesToFlushedPageCanBeStoredAgain)
{
  bloom_.Add("A", 1);
  bloom_.Add("B", 20);

  ASSERT_EQ(1, bloom_.TrimCache());

  bloom_.Add("C", 2);

  ASSERT_EQ(1, bloom_.TrimCache()); // <- dropped from memory here

  ASSERT_TRUE(bloom_.Match("A", 1, 20));
  ASSERT_TRUE(bloom_.Match("B", 1, 20));
  ASSERT_TRUE(bloom_.Match("C", 1, 20));
}

TEST_F(HistoricalBloomFilterTests, CheckFlushingToDisk)
{
  ASSERT_TRUE(bloom_.Add("A", 1));

  // trigger the flush to disk
  bloom_.Flush();

  HistoricalBloomFilter loaded{HistoricalBloomFilter::Mode::LOAD_DATABASE,
                               "h-bloom-tests.db",
                               "h-bloom-tests.index.db",
                               "h-bloom-tests.meta.db",
                               WINDOW_SIZE,
                               MAX_CACHED};

  ASSERT_TRUE(bloom_.Match("A", 1, 1)); // this should already be in memory so it is fine
  ASSERT_TRUE(loaded.Match("A", 1, 1)); // <- actual test
}

TEST_F(HistoricalBloomFilterTests, DetectLoadFailure)
{
  ASSERT_TRUE(bloom_.Add("A", 1));

  // trigger the flush to disk
  bloom_.Flush();

  ASSERT_THROW(HistoricalBloomFilter a(HistoricalBloomFilter::Mode::LOAD_DATABASE,
                               "h-bloom-tests.db",
                               "h-bloom-tests.index.db",
                               "h-bloom-tests.meta.db",
                               WINDOW_SIZE + 1, // <- the window size if different
                               MAX_CACHED), std::runtime_error);
}


TEST(AltHistoricalBloomFilterTests, CheckIntegrityOnReload)
{
  // create the bloom filter
  HistoricalBloomFilter bloom1{HistoricalBloomFilter::Mode::NEW_DATABASE,
                               "h-bloom-tests.db",
                               "h-bloom-tests.index.db",
                               "h-bloom-tests.meta.db",
                               10,
                               1};

  // add the entries into the bloom filter as usual
  ASSERT_TRUE(bloom1.Add("A", 1));
  ASSERT_TRUE(bloom1.Add("B", 11));
  ASSERT_TRUE(bloom1.Add("C", 21));
  ASSERT_TRUE(bloom1.Add("D", 31));

  ASSERT_TRUE(bloom1.Match("A", 1, 60));
  ASSERT_TRUE(bloom1.Match("B", 1, 60));
  ASSERT_TRUE(bloom1.Match("C", 1, 60));
  ASSERT_TRUE(bloom1.Match("D", 1, 60));

  // trim the cache
  ASSERT_EQ(bloom1.TrimCache(), 3);

  // start adding more elements
  ASSERT_TRUE(bloom1.Add("E", 41));
  ASSERT_TRUE(bloom1.Add("F", 51));

  // simulate a crash by simply trying to restore the previous database from its current state on
  // disk. In this case, we expect the last page that was flushed to be page 2 (index range [20-30) )
  HistoricalBloomFilter bloom2{HistoricalBloomFilter::Mode::LOAD_DATABASE,
                               "h-bloom-tests.db",
                               "h-bloom-tests.index.db",
                               "h-bloom-tests.meta.db",
                               10,
                               1};

  // check the last flushed buckets
  ASSERT_EQ(bloom2.last_flushed_bucket(), 2);
  ASSERT_EQ(bloom1.last_flushed_bucket(), bloom2.last_flushed_bucket());

  // check that the filter behaves as it reports, i.e. it has no knowledge of D, E & F
  ASSERT_TRUE(bloom2.Match("A", 1, 60));
  ASSERT_TRUE(bloom2.Match("B", 1, 60));
  ASSERT_TRUE(bloom2.Match("C", 1, 60));
  ASSERT_FALSE(bloom2.Match("D", 1, 60));
  ASSERT_FALSE(bloom2.Match("E", 1, 60));
  ASSERT_FALSE(bloom2.Match("F", 1, 60));
}

}