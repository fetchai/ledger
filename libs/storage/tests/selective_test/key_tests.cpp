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

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "storage/key.hpp"
#include "testing/bitset_array_conversion.hpp"
#include "testing/common_testing_functionality.hpp"

#include <gtest/gtest.h>

using namespace testing;
using namespace fetch;
using namespace fetch::storage;
using namespace fetch::crypto;
using namespace fetch::testing;

using ByteArray          = fetch::byte_array::ByteArray;
using ConstByteArray     = fetch::byte_array::ConstByteArray;
using DefaultKey         = fetch::storage::Key<256>;
using DefaultBitset      = std::bitset<DefaultKey::BITS>;
using DefaultArray       = ArrayB<uint64_t, DefaultKey::BITS>;
using UnorderedSetBitset = std::unordered_set<DefaultBitset>;

TEST(new_key_test, test_compare_keys_shifted_by_single_bit__triangular_formation)
{
  std::vector<DefaultKey> keys;

  DefaultBitset key_val{0};
  key_val.flip();

  for (std::size_t i = 0; i < key_val.size(); ++i)
  {
    auto const bs_key{key_val >> i};

    keys.emplace_back(to_ByteArray(bs_key));
    auto const &key = keys.back();

    if (i == 0)
    {
      continue;
    }

    auto const &last_key{keys[i - 1]};

    // auto const &last_bs_key{bs_keys[i-1]};
    // auto const &last_arr_key{arr_keys[i-1]};
    // std::cout << i << ": last key bs :" << last_bs_key.to_string() << std::endl;
    // std::cout << i << ": curr key bs :" << bs_key.to_string() << std::endl;
    // std::cout << i << ": last key arr:" << last_arr_key << std::endl;
    // std::cout << i << ": curr key arr:" << k_arr << std::endl;

    int pos{0};

    // TESTED PRODUCTION CODE:
    auto res = key.Compare(key, pos, static_cast<uint16_t>(key.size_in_bits()));

    // EXPECTATIONS:
    // Comparing the the key with itself, expected identity result
    EXPECT_EQ(key.size_in_bits(), pos);
    EXPECT_EQ(0, res);

    // Comparing *current* key against *pevious* key which is BIGGER by value
    res = key.Compare(last_key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(key.size_in_bits() - i, pos);
    EXPECT_EQ(-1, res);

    // Reciprocally comparing *pevious* key to *current* key which is SMALLER by value
    res = last_key.Compare(key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(key.size_in_bits() - i, pos);
    EXPECT_EQ(1, res);
  }
}

TEST(new_key_test, test_compare_for_keys_whis_shifted_single_zero_by_one_bit__moving_zero_formation)
{
  std::vector<DefaultKey>    keys;
  std::vector<DefaultArray>  arr_keys;
  std::vector<DefaultBitset> bs_keys;

  DefaultBitset key_val{0};
  key_val = 1;

  for (std::size_t i = 0; i < key_val.size(); ++i)
  {
    bs_keys.emplace_back(~(key_val << i));
    auto const &bs_key = bs_keys.back();

    keys.emplace_back(to_ByteArray(bs_key));
    auto const &key = keys.back();

    if (i == 0)
    {
      continue;
    }

    auto const &last_key{keys[i - 1]};

    // auto const &last_bs_key{bs_keys[i-1]};
    // auto const &last_arr_key{arr_keys[i-1]};
    // std::cout << i << ": last key bs :" << last_bs_key.to_string() << std::endl;
    // std::cout << i << ": curr key bs :" << bs_key.to_string() << std::endl;
    // std::cout << i << ": last key arr:" << last_arr_key << std::endl;
    // std::cout << i << ": curr key arr:" << k_arr << std::endl;

    int pos{0};
    // Comparing the the key with itself, expected identity result
    auto res = key.Compare(key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(key.size_in_bits(), pos);
    EXPECT_EQ(0, res);

    // Comparing *current* key against *pevious* key which is SMALLER by value
    res = key.Compare(last_key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(i - 1, pos);
    EXPECT_EQ(1, res);

    // Reciprocally comparing *pevious* key to *current* key which is BIGGER by value
    res = last_key.Compare(key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(i - 1, pos);
    EXPECT_EQ(-1, res);
  }
}

TEST(new_key_test, equality_comparison_operator)
{
  auto const       start_bs_key{~DefaultBitset{0}};
  DefaultKey const start_key{to_ByteArray(start_bs_key)};

  for (std::size_t i = 1; i < start_key.size_in_bits(); ++i)
  {
    auto const shifted_key_ByteArray{to_ByteArray(start_bs_key >> i)};

    DefaultKey const key{shifted_key_ByteArray};
    DefaultKey const key_copy{shifted_key_ByteArray};

    EXPECT_TRUE(key == key_copy);
    EXPECT_TRUE(key_copy == key);

    EXPECT_FALSE(start_key == key);
    EXPECT_FALSE(key == start_key);
  }
}

// Test that closely correlated keys are found to be unique
TEST(new_key_test, correlated_keys_are_unique)
{
  auto unique_hashes{GenerateUniqueHashes(1000)};

  std::vector<DefaultKey> seen_keys;
  seen_keys.reserve(unique_hashes.size());

  for (auto const &hash : unique_hashes)
  {
    DefaultKey const key{hash};
    auto const       itr = std::find(seen_keys.begin(), seen_keys.end(), key);

    EXPECT_EQ(seen_keys.cend(), itr);  // Expected *NOT* to be found

    seen_keys.emplace_back(std::move(key));
  }
}

TEST(new_key_test, correlated_keys_are_unique_1)
{
  auto const key_val{~DefaultBitset{0}};

  std::vector<DefaultKey> seen_keys;
  seen_keys.reserve(key_val.size());

  for (std::size_t i = 0; i < key_val.size(); ++i)
  {
    auto       bs_key = (key_val >> i);
    auto const k_arr{to_array<DefaultArray::value_type>(bs_key)};

    byte_array::ConstByteArray const cba_key{
        reinterpret_cast<byte_array::ConstByteArray::container_type const *>(k_arr.data()),
        k_arr.size() * sizeof(decltype(k_arr)::value_type)};

    DefaultKey const key{cba_key};
    auto const       itr = std::find(seen_keys.begin(), seen_keys.end(), key);

    EXPECT_EQ(seen_keys.cend(), itr);  // Expected *NOT* to be found

    seen_keys.emplace_back(std::move(key));
  }
}

TEST(new_key_test, test_comparison_using_last_bit_value__moving_zero_formation)
{
  DefaultKey const ref_key{to_ByteArray(~DefaultBitset{0})};

  DefaultBitset const bs_key_val{1};
  for (std::size_t i = 0; i < bs_key_val.size(); ++i)
  {
    auto const bs_key{~(bs_key_val << i)};

    DefaultKey key{to_ByteArray(bs_key)};

    int pos{0};

    auto const last_bit_position_WITH_expected_difference{static_cast<uint16_t>(i)};
    // Comparing *current* key against *ref* key which is BIGGER by value
    auto res = key.Compare(ref_key, pos, last_bit_position_WITH_expected_difference);
    EXPECT_EQ(i, pos);
    EXPECT_EQ(-1, res);

    // Reciprocally comparing *ref* key to *current* key which is SMALLER by value
    res = ref_key.Compare(key, pos, last_bit_position_WITH_expected_difference);
    EXPECT_EQ(i, pos);
    EXPECT_EQ(1, res);

    auto const last_bit_position{static_cast<uint16_t>(i + 1)};
    // Comparing *current* key against *ref* key which is BIGGER by value
    res = key.Compare(ref_key, pos, last_bit_position);
    EXPECT_EQ(i, pos);
    EXPECT_EQ(-1, res);

    // Reciprocally comparing *ref* key to *current* key which is SMALLER by value
    res = ref_key.Compare(key, pos, last_bit_position);
    EXPECT_EQ(i, pos);
    EXPECT_EQ(1, res);

    if (i > 1)
    {
      auto const last_bit_position_BEFORE_actual_difference{static_cast<uint16_t>(i - 1)};
      // Comparing *current* key against *ref* key which are the same when
      // given the *last position* limitation.
      res = key.Compare(ref_key, pos, last_bit_position_BEFORE_actual_difference);
      EXPECT_EQ(last_bit_position_BEFORE_actual_difference, pos);
      EXPECT_EQ(1, res);

      // Reciprocally comparing *ref* key to *current* key which are the same when
      // given the *last position* limitation.
      res = ref_key.Compare(key, pos, last_bit_position_BEFORE_actual_difference);
      EXPECT_EQ(last_bit_position_BEFORE_actual_difference, pos);
      EXPECT_EQ(1, res);
    }
  }
}

TEST(new_key_test, test_comparison_using_last_bit_value__triangular_formation)
{
  DefaultBitset const bs_key_val{~DefaultBitset{0}};

  DefaultKey prev_key;

  for (std::size_t i = 0; i < bs_key_val.size(); ++i)
  {
    DefaultKey key{to_ByteArray(bs_key_val << i)};

    if (i > 0)
    {
      int pos{0};

      auto const last_bit_position_WITH_expected_difference{static_cast<uint16_t>(i)};
      // Comparing *current* key against *prev* key which is BIGGER by value
      auto res = key.Compare(prev_key, pos, last_bit_position_WITH_expected_difference);
      EXPECT_EQ(i - 1, pos);
      EXPECT_EQ(-1, res);

      // Reciprocally comparing *prev* key to *current* key which is SMALLER by value
      res = prev_key.Compare(key, pos, last_bit_position_WITH_expected_difference);
      EXPECT_EQ(i - 1, pos);
      EXPECT_EQ(1, res);

      auto const last_bit_position{static_cast<uint16_t>(i + 1)};
      // Comparing *current* key against *prev* key which is BIGGER by value
      res = key.Compare(prev_key, pos, last_bit_position);
      EXPECT_EQ(i - 1, pos);
      EXPECT_EQ(-1, res);

      // Reciprocally comparing *prev* key to *current* key which is SMALLER by value
      res = prev_key.Compare(key, pos, last_bit_position);
      EXPECT_EQ(i - 1, pos);
      EXPECT_EQ(1, res);

      if (i > 1)
      {
        auto const last_bit_position_BEFORE_actual_difference{static_cast<uint16_t>(i - 2)};
        // Comparing *current* key against *prev* key which are the same when
        // given the *last position* limitation.
        res = key.Compare(prev_key, pos, last_bit_position_BEFORE_actual_difference);
        EXPECT_EQ(last_bit_position_BEFORE_actual_difference, pos);
        EXPECT_EQ(-1, res);

        // Reciprocally comparing *prev* key to *current* key which are the same when
        // given the *last position* limitation.
        res = prev_key.Compare(key, pos, last_bit_position_BEFORE_actual_difference);
        EXPECT_EQ(last_bit_position_BEFORE_actual_difference, pos);
        EXPECT_EQ(-1, res);
      }
    }

    prev_key = std::move(key);
  }
}
