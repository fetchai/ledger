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
using DefaultArray       = ArrayBites<uint64_t, DefaultKey::BITS>;
using UnorderedSetBitset = std::unordered_set<DefaultBitset>;

class NewKeyTest : public Test
{
protected:
  template <typename CONTAINER_TYPE>
  void test_correlated_keys_are_unique(CONTAINER_TYPE &&unique_hashes)
  {
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
};

TEST_F(NewKeyTest, test_compare_keys_triang_bit_shift)
{
  std::vector<DefaultKey> keys;

  auto const key_val{~DefaultBitset{0}};

  for (std::size_t i = 0; i < key_val.size(); ++i)
  {
    auto const bs_key{key_val >> i};

    keys.emplace_back(to_ByteArray(bs_key));
    auto const &key = keys.back();
    //`bs_keys` container will contain (values *DISPLAYED* in *BIG* Endian encoding):
    // bs_keys[0]   = 111111...11
    // bs_keys[1]   = 011111...11
    // bs_keys[2]   = 001111...11
    // bs_keys[3]   = 000111...11
    // bs_keys[4]   = 000011...11
    // bs_keys[5]   = 000001...11
    //             ...
    // bs_keys[255] = 000000...01

    if (i == 0)
    {
      continue;
    }

    auto const &last_key{keys[i - 1]};

    int pos{0};

    // TESTED PRODUCTION CODE:
    auto res = key.Compare(key, pos, static_cast<uint16_t>(key.size_in_bits()));

    // EXPECTATIONS:
    // Comparing the the key with itself, expected identity result
    EXPECT_EQ(key.size_in_bits(), pos);
    EXPECT_EQ(0, res);

    // Comparing *current* key against *previous* key which is BIGGER by value
    res = key.Compare(last_key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(key.size_in_bits() - i, pos);
    EXPECT_EQ(-1, res);

    // Reciprocally comparing *previous* key to *current* key which is SMALLER by value
    res = last_key.Compare(key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(key.size_in_bits() - i, pos);
    EXPECT_EQ(1, res);
  }
}

TEST_F(NewKeyTest, test_compare_for_keys_with_moving_zero)
{
  std::vector<DefaultKey>    keys;
  std::vector<DefaultArray>  arr_keys;
  std::vector<DefaultBitset> bs_keys;

  DefaultBitset const key_val{1};

  for (std::size_t i = 0; i < key_val.size(); ++i)
  {
    bs_keys.emplace_back(~(key_val << i));
    auto const &bs_key = bs_keys.back();
    //`bs_keys` container will contain (values *DISPLAYED* in *BIG* Endian encoding):
    // bs_keys[  0]   = 11... 111110
    // bs_keys[  1]   = 11... 111101
    // bs_keys[  2]   = 11... 111011
    // bs_keys[  3]   = 11... 110111
    // bs_keys[  4]   = 11... 101111
    // bs_keys[  5]   = 11... 011111
    //             ...
    // bs_keys[254]   = 10... 111111
    // bs_keys[255]   = 01... 111111

    keys.emplace_back(to_ByteArray(bs_key));
    auto const &key = keys.back();

    if (i == 0)
    {
      continue;
    }

    auto const &last_key{keys[i - 1]};

    int pos{0};
    // Comparing the the key with itself, expected identity result
    auto res = key.Compare(key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(key.size_in_bits(), pos);
    EXPECT_EQ(0, res);

    // Comparing *current* key against *previous* key which is SMALLER by value
    res = key.Compare(last_key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(i - 1, pos);
    EXPECT_EQ(1, res);

    // Reciprocally comparing *previous* key to *current* key which is BIGGER by value
    res = last_key.Compare(key, pos, static_cast<uint16_t>(key.size_in_bits()));
    EXPECT_EQ(i - 1, pos);
    EXPECT_EQ(-1, res);
  }
}

TEST_F(NewKeyTest, equality_comparison_operator)
{
  auto const       start_bs_key{~DefaultBitset{0}};
  DefaultKey const start_key{to_ByteArray(start_bs_key)};

  for (std::size_t i = 1; i < start_key.size_in_bits(); ++i)
  {
    auto const shifted_key_ByteArray{to_ByteArray(start_bs_key >> i)};
    //`shifted_key_ByteArray` container will contain (values *DISPLAYED* in *BIG* Endian encoding):
    // i=0:   shifted_key_ByteArray = 111111...11
    // i=1:   shifted_key_ByteArray = 011111...11
    // i=2:   shifted_key_ByteArray = 001111...11
    // i=3:   shifted_key_ByteArray = 000111...11
    // i=4:   shifted_key_ByteArray = 000011...11
    // i=5:   shifted_key_ByteArray = 000001...11
    //              ...
    // i=255: shifted_key_ByteArray = 000000...01

    DefaultKey const key{shifted_key_ByteArray};
    DefaultKey const key_copy{shifted_key_ByteArray};

    EXPECT_TRUE(key == key_copy);
    EXPECT_TRUE(key_copy == key);

    EXPECT_FALSE(start_key == key);
    EXPECT_FALSE(key == start_key);
  }
}

// Test that closely correlated keys are found to be unique
TEST_F(NewKeyTest, correlated_keys_are_unique)
{
  test_correlated_keys_are_unique(GenerateUniqueHashes(1000));
}

TEST_F(NewKeyTest, correlated_keys_are_unique_triang_form)
{
  auto const key_val{~DefaultBitset{0}};  //= 111...11 (bin) = 0xfff...ff (hex)
  std::vector<byte_array::ConstByteArray> unique_hashes;
  unique_hashes.reserve(key_val.size());

  for (std::size_t i = 0; i < key_val.size(); ++i)
  {
    auto const hash = (key_val >> i);
    unique_hashes.emplace_back(to_ByteArray(hash));
  }
  //`unique_hashes` container will contain (values *DISPLAYED* in *BIG* Endian encoding):
  // unique_hashes[0]   = 111111...11
  // unique_hashes[1]   = 011111...11
  // unique_hashes[2]   = 001111...11
  // unique_hashes[3]   = 000111...11
  // unique_hashes[4]   = 000011...11
  // unique_hashes[5]   = 000001...11
  //             ...
  // unique_hashes[255] = 000000...01

  test_correlated_keys_are_unique(std::move(unique_hashes));
}

TEST_F(NewKeyTest, test_comparison_using_last_bit_value_moving_zero_formation)
{
  DefaultKey const ref_key{to_ByteArray(~DefaultBitset{0})};  //=111...11 (bin) = 0xfff..ff (hex)

  DefaultBitset const bs_key_val{1};
  for (std::size_t i = 0; i < bs_key_val.size(); ++i)
  {
    auto const bs_key{~(bs_key_val << i)};
    //`bs_key` container will contain (values *DISPLAYED* in *BIG* Endian encoding):
    // i=  0: bs_key = 11... 111110
    // i=  1: bs_key = 11... 111101
    // i=  2: bs_key = 11... 111011
    // i=  3: bs_key = 11... 110111
    // i=  4: bs_key = 11... 101111
    // i=  5: bs_key = 11... 011111
    //              ...
    // i=254: bs_key = 10... 111111
    // i=255: bs_key = 01... 111111

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

TEST_F(NewKeyTest, test_comparison_using_last_bit_value_triangular_formation)
{
  DefaultBitset const bs_key_val{~DefaultBitset{0}};

  DefaultKey prev_key;

  for (std::size_t i = 0; i < bs_key_val.size(); ++i)
  {
    DefaultKey key{to_ByteArray(bs_key_val << i)};
    //`bs_key` container will contain (values *DISPLAYED* in *BIG* Endian encoding):
    // i=  0: key = 11... 111111
    // i=  1: key = 11... 111110
    // i=  2: key = 11... 111100
    // i=  3: key = 11... 111000
    // i=  4: key = 11... 110000
    // i=  5: key = 11... 100000
    //           ...
    // i=254: key = 11... 000000
    // i=255: key = 10... 000000

    if (i > 0)
    {
      int pos{0};

      auto const last_bit_position_WITH_expected_difference{static_cast<uint16_t>(i - 1)};
      // Comparing *current* key against *prev* key which is BIGGER by value
      auto res = key.Compare(prev_key, pos, last_bit_position_WITH_expected_difference);
      EXPECT_EQ(last_bit_position_WITH_expected_difference, pos);
      EXPECT_EQ(-1, res);

      // Reciprocally comparing *prev* key to *current* key which is SMALLER by value
      res = prev_key.Compare(key, pos, last_bit_position_WITH_expected_difference);
      EXPECT_EQ(last_bit_position_WITH_expected_difference, pos);
      EXPECT_EQ(1, res);

      auto const last_bit_position{static_cast<uint16_t>(i)};
      // Comparing *current* key against *prev* key which is BIGGER by value
      res = key.Compare(prev_key, pos, last_bit_position);
      EXPECT_EQ(last_bit_position_WITH_expected_difference, pos);
      EXPECT_EQ(-1, res);

      // Reciprocally comparing *prev* key to *current* key which is SMALLER by value
      res = prev_key.Compare(key, pos, last_bit_position);
      EXPECT_EQ(last_bit_position_WITH_expected_difference, pos);
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
