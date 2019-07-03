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

#include "core/bloom_filter.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include "gmock/gmock.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

namespace {

using namespace fetch;

std::vector<std::size_t> double_length_as_hash(internal::HashSourceFactory::Bytes const &input)
{
  return {2 * input.size()};
}

std::vector<std::size_t> length_powers_as_hash(internal::HashSourceFactory::Bytes const &input)
{
  auto const size = input.size();

  return {size, size * size, size * size * size};
}

std::vector<std::size_t> raw_data_as_hash(internal::HashSourceFactory::Bytes const &input)
{
  auto start = reinterpret_cast<std::size_t const *>(input.pointer());

  const auto               size_in_bytes = input.size();
  std::vector<std::size_t> output((size_in_bytes + sizeof(std::size_t) - 1) / sizeof(std::size_t));

  for (std::size_t i = 0; i < output.size(); ++i)
  {
    output[i] = *(start + i);
  }

  return output;
}

class HashSourceTests : public ::testing::Test
{
public:
  HashSourceTests()
    : hash_source_factory({double_length_as_hash, length_powers_as_hash, raw_data_as_hash})
    // ASCII '3' == 0x33u, 'D' == 0x44u, 'w' == 0x77u
    , input("wD3D3D3D333ww")
    , expected_output{2 * input.size(), input.size(), input.size() * input.size(),
                      input.size() * input.size() * input.size(),
                      // First 8 bytes of input
                      0x4433443344334477ull,
                      // Second 8 bytes of input
                      0x7777333333ull}
    , hash_source(hash_source_factory(input))
  {}

  internal::HashSourceFactory const        hash_source_factory;
  internal::HashSourceFactory::Bytes const input;
  std::vector<std::size_t> const           expected_output;
  internal::HashSource const               hash_source;
};

TEST_F(HashSourceTests, hash_source_supports_iterator_ranges_and_evaluates_hashes_in_order)
{
  std::vector<std::size_t> output_from_range(hash_source.cbegin(), hash_source.cend());

  EXPECT_EQ(output_from_range, expected_output);
}

TEST_F(HashSourceTests, hash_source_supports_range_for_loops_and_evaluates_hashes_in_order)
{
  std::vector<std::size_t> output_from_loop;
  for (std::size_t hash : hash_source)
  {
    output_from_loop.push_back(hash);
  }

  EXPECT_EQ(output_from_loop, expected_output);
}

TEST_F(HashSourceTests, one_hash_source_may_be_traversed_multiple_times)
{
  std::vector<std::size_t> output_from_range1(hash_source.cbegin(), hash_source.cend());
  std::vector<std::size_t> output_from_range2(hash_source.begin(), hash_source.end());

  std::vector<std::size_t> output_from_loop1;
  std::vector<std::size_t> output_from_loop2;
  for (std::size_t hash : hash_source)
  {
    output_from_loop1.push_back(hash);
  }
  for (std::size_t hash : hash_source)
  {
    output_from_loop2.push_back(hash);
  }

  EXPECT_EQ(output_from_loop1, expected_output);
  EXPECT_EQ(output_from_loop2, expected_output);
  EXPECT_EQ(output_from_range1, expected_output);
  EXPECT_EQ(output_from_range2, expected_output);
}

class BloomFilterTests : public ::testing::Test
{
public:
  BloomFilterTests()
    : filter({double_length_as_hash, length_powers_as_hash, raw_data_as_hash})
    , filter_weak_hashing({double_length_as_hash, length_powers_as_hash})
  {}

  BasicBloomFilter filter;
  BasicBloomFilter filter_weak_hashing;
};

TEST_F(BloomFilterTests, foo0)  //???
{
  EXPECT_FALSE(filter.Match("abc"));
}

TEST_F(BloomFilterTests, foo)  //???
{
  filter.Add("abc");
  EXPECT_TRUE(filter.Match("abc"));
}

TEST_F(BloomFilterTests, foo2)  //???
{
  filter.Add("abc");
  EXPECT_FALSE(filter.Match("xyz"));
}

TEST_F(BloomFilterTests, foo99)  //???
{
  filter.Add("abc");
  filter.Add("xyz");
  EXPECT_TRUE(filter.Match("abc"));
  EXPECT_TRUE(filter.Match("xyz"));
  EXPECT_FALSE(filter.Match("klm"));
}

TEST_F(BloomFilterTests, foo3)  //???
{
  filter_weak_hashing.Add("abc");

  EXPECT_TRUE(filter_weak_hashing.Match("xyz"));
}

}  // namespace

//???lazy hash evaluation
//???experimental feature flag
//???limit friend classes
//???add linear combinations of hashes
//???limit size of hash source
//???spurious copies, moves i could make?
//???add special functions, default/delete where appropriate
//???noexcept functions? hashes? constexpr?
//???move false positives tracking to bloom filter
//???adaptable filter - size, number of hashes
//???hash linear combinations
//???reuse openssl contexts
//???factory ask for specific number of bits, cutoff hasher execution
//???documentation comments
//???OpenSslHasher move to crypto, make reusable
//???nasty ternary in constellation ctor - make static factory method eg
// createExperimentalBloomFilter(festureflag)
