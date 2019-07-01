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
#include "core/byte_array/encoders.hpp"
#include "core/random/lfg.hpp"
#include "storage/key.hpp"
#include "storage/key_value_index.hpp"

#include "gtest/gtest.h"

#include <algorithm>

namespace {

using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;

using Index = fetch::storage::KeyValueIndex<>;
using RNG   = fetch::random::LaggedFibonacciGenerator<>;

struct TestData
{
  ByteArray key;
  uint64_t  value;

  TestData(ByteArray k, uint64_t v)
    : key{std::move(k)}
    , value{v}
  {}
};

using TestDataArray = std::vector<TestData>;
using ReferenceMap  = std::map<ConstByteArray, uint64_t>;

TestDataArray GenerateTestData(RNG &rng, ReferenceMap &ref_map)
{
  static constexpr std::size_t NUM_ENTRIES        = 5;
  static constexpr std::size_t IDENTITY_BIT_SIZE  = 256;
  static constexpr std::size_t IDENTITY_BYTE_SIZE = IDENTITY_BIT_SIZE / 8u;
  static constexpr std::size_t IDENTITY_WORD_SIZE = IDENTITY_BYTE_SIZE / sizeof(RNG::random_type);

  TestDataArray values;

  for (std::size_t i = 0; i < NUM_ENTRIES; ++i)
  {
    ByteArray key;
    key.Resize(IDENTITY_BYTE_SIZE);

    // generate a random key
    auto *key_raw = reinterpret_cast<RNG::random_type *>(key.pointer());
    for (std::size_t j = 0; j < IDENTITY_WORD_SIZE; ++j)
    {
      key_raw[j] = rng();
    }

    // ensure the key that has been generated is actually unique
    if (ref_map.find(key) != ref_map.end())
    {
      continue;
    }

    // generate a random value
    uint64_t const value = rng();

    // update the reference map and the output data array
    ref_map[key] = value;
    values.emplace_back(key, value);
  }

  return values;
}

using KV = fetch::storage::KeyValuePair<>;

TEST(versioned_kvi_gtest, basic_test)
{
  static constexpr std::size_t BOOKMARK_INTERVAL = 2;

  Index key_value_index;

  TestDataArray bookmarks;
  ReferenceMap  ref_map;

  RNG rng;

  // Generate a series of test data with unique key values
  TestDataArray values = GenerateTestData(rng, ref_map);

  // Insert the generate values into the index
  key_value_index.New("test1.db", "diff.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];

    // update the index with the key and the value
    key_value_index.Set(val.key, val.value, val.key);

    // with a given interval make a bookmark
    if ((i % BOOKMARK_INTERVAL) == 0)
    {
      auto const hash  = key_value_index.Hash();
      auto const index = key_value_index.Commit();

      // add to bookmark list
      bookmarks.emplace_back(hash, index);

      ASSERT_EQ(hash, key_value_index.Hash())
          << "Expected hash to be the same before and after commit";
    }
  }

  // Checking values
  for (auto const &value : values)
  {
    // lookup the value from the index
    uint64_t const stored_value = key_value_index.Get(value.key);

    // check that it is as expected
    ASSERT_EQ(stored_value, value.value);
  }

  // Reverting
  std::reverse(bookmarks.begin(), bookmarks.end());
  for (auto &b : bookmarks)
  {
    key_value_index.Revert(b.value);

    auto const restored_hash = key_value_index.Hash();

    ASSERT_EQ(b.key, restored_hash);
  }
}

}  // namespace
