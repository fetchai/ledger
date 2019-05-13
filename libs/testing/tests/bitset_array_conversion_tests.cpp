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

#include "testing/bitset_array_conversion.hpp"

#include <gtest/gtest.h>

namespace fetch {
namespace testing {
namespace {

using namespace testing;

constexpr std::size_t BITS{256};

using DefaultArray  = ArrayB<uint64_t, BITS>;
using DefaultBitset = std::bitset<BITS>;

TEST(array_bitset_test, test_conversion_bitset_to_array_and_back)
{
  std::vector<DefaultArray>  arr_keys;
  std::vector<DefaultBitset> bs_keys;

  auto const starting_bitset{~DefaultBitset{0}};  //= 111...1 (bin) = 0xff...f (hex)

  //`bs_keys` container will contain (values *DISPLAYED* in *BIG* Endian encoding):
  // bs_keys[0]   = 111111...11
  // bs_keys[1]   = 011111...11
  // bs_keys[2]   = 001111...11
  // bs_keys[3]   = 000111...11
  // bs_keys[4]   = 000011...11
  // bs_keys[5]   = 000001...11
  //             ...
  // bs_keys[255] = 000000...01

  for (std::size_t i = 0; i < starting_bitset.size(); ++i)
  {
    bs_keys.emplace_back(starting_bitset >> i);
    auto const &current_bitset = bs_keys.back();

    DefaultArray interm_arr{to_array<DefaultArray::value_type>(current_bitset)};
    arr_keys.emplace_back(interm_arr);

    DefaultBitset regenerated_bitset{to_bitset(interm_arr)};

    // Verifying that full circle conversion (bitset -> array -> bitset) gives
    // the same value as original bitset.
    ASSERT_EQ(current_bitset, regenerated_bitset);

    // Not strictly necessary, just verifying the behaviour of the std::bitset container,
    // specifically that XOR bs_keys[i] ^ bs_keys[i-1] has expected result:
    if (i > 0)
    {
      auto const &previous_bitset = bs_keys[i - 1];
      ASSERT_EQ(DefaultBitset{1} << (starting_bitset.size() - i), current_bitset ^ previous_bitset);
    }
  }
}

TEST(array_bitset_test, test_conversion_bitset_to_ByteArray)
{
  auto const starting_bitset{~DefaultBitset{0}};  //= 111...1 (bin) = 0xff...f (hex)

  //`bs_key` container will contain (values *DISPLAYED* in *BIG* Endian encoding):
  // i=0:   bs_key = 111111...11
  // i=1:   bs_key = 011111...11
  // i=2:   bs_key = 001111...11
  // i=3:   bs_key = 000111...11
  // i=4:   bs_key = 000011...11
  // i=5:   bs_key = 000001...11
  //              ...
  // i=255: bs_key = 000000...01

  for (std::size_t i = 0; i < starting_bitset.size(); ++i)
  {
    auto const bs_key{starting_bitset >> i};

    // PRODUCTION code
    auto const result = to_ByteArray(bs_key);

    // BASIC EXPECTATION
    EXPECT_EQ(bs_key.size() / 8, result.size());

    DefaultArray k_arr{to_array<DefaultArray::value_type>(bs_key)};
    ASSERT_EQ(k_arr.size() * sizeof(decltype(k_arr)::value_type), result.size());

    auto const expected_bytes{
        reinterpret_cast<byte_array::ByteArray::container_type const *>(k_arr.data())};
    auto const compare_result = std::memcmp(result.pointer(), expected_bytes, result.size());
    // PRIMARY expectation
    EXPECT_EQ(0, compare_result);
  }
}

}  // namespace
}  // namespace testing
}  // namespace fetch
