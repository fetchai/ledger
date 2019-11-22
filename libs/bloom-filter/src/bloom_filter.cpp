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

#include "bloom_filter/bloom_filter.hpp"
#include "core/bitvector.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"
#include "crypto/hash.hpp"
#include "crypto/md5.hpp"
#include "crypto/sha1.hpp"
#include "crypto/sha512.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

constexpr std::size_t const INITIAL_SIZE_IN_BITS = 8 * 1 * 1024 * 1024;

namespace fetch {
namespace internal {

HashSourceFactory::HashSourceFactory(HashSourceFactory::Functions hash_functions)
  : hash_functions_(std::move(hash_functions))
{}

HashSource HashSourceFactory::operator()(fetch::byte_array::ConstByteArray const &element) const
{
  return HashSource(this->hash_functions_, element);
}

HashSource::HashSource(HashSourceFactory::Functions const &     hash_functions,
                       fetch::byte_array::ConstByteArray const &input)
{
  // TODO(LDGR-319): lazily evaluate hashes
  for (auto const &fn : hash_functions)
  {
    auto const hashes = fn(input);
    data_.insert(data_.cend(), hashes.cbegin(), hashes.cend());
  }
}

HashSource::HashSourceIterator HashSource::begin() const
{
  return HashSource::HashSourceIterator{this, 0u};
}

HashSource::HashSourceIterator HashSource::end() const
{
  return HashSource::HashSourceIterator{this, data_.size()};
}

HashSource::HashSourceIterator HashSource::cbegin() const
{
  return HashSource::HashSourceIterator{this, 0u};
}

HashSource::HashSourceIterator HashSource::cend() const
{
  return HashSource::HashSourceIterator{this, data_.size()};
}

std::size_t HashSource::getHash(std::size_t index) const
{
  return data_[index];
}

bool HashSource::HashSourceIterator::operator!=(HashSource::HashSourceIterator const &other) const
{
  return !(operator==(other));
}

bool HashSource::HashSourceIterator::operator==(HashSource::HashSourceIterator const &other) const
{
  return source_ == other.source_ && hash_index_ == other.hash_index_;
}

HashSource::HashSourceIterator &HashSource::HashSourceIterator::operator++()
{
  ++hash_index_;

  return *this;
}

std::size_t HashSource::HashSourceIterator::operator*() const
{
  return source_->getHash(hash_index_);
}

HashSource::HashSourceIterator::HashSourceIterator(HashSource const *source, std::size_t index)
  : hash_index_{index}
  , source_{source}
{}

namespace {

HashSource::Hashes raw_data(fetch::byte_array::ConstByteArray const &input)
{
  auto start = reinterpret_cast<std::size_t const *>(input.pointer());

  auto const size_in_bytes = input.size();

  HashSource::Hashes output((size_in_bytes + sizeof(std::size_t) - 1) / sizeof(std::size_t));

  for (std::size_t i = 0; i < output.size(); ++i)
  {
    output[i] = *(start + i);
  }

  return output;
}

template <typename Hasher>
HashSource::Hashes HashSourceFunction(fetch::byte_array::ConstByteArray const &input)
{
  HashSource::Hashes output((Hasher::size_in_bytes + sizeof(std::size_t) - 1) /
                            sizeof(std::size_t));
  crypto::Hash<Hasher>(input.pointer(), input.size(), reinterpret_cast<uint8_t *>(output.data()));

  return output;
}

HashSource::Hashes fnv(fetch::byte_array::ConstByteArray const &input)
{
  return internal::HashSourceFunction<crypto::FNV>(input);
}

HashSource::Hashes md5(fetch::byte_array::ConstByteArray const &input)
{
  return internal::HashSourceFunction<crypto::MD5>(input);
}

}  // namespace

}  // namespace internal

BasicBloomFilter::Functions const default_hash_functions{internal::raw_data, internal::fnv,
                                                         internal::md5};

BasicBloomFilter::BasicBloomFilter()
  : bits_(INITIAL_SIZE_IN_BITS)
  , hash_source_factory_(default_hash_functions)
{}

BasicBloomFilter::BasicBloomFilter(Functions const &functions)
  : bits_(INITIAL_SIZE_IN_BITS)
  , hash_source_factory_(functions)
{}

std::pair<bool, std::size_t> BasicBloomFilter::Match(
    fetch::byte_array::ConstByteArray const &element) const
{
  auto const  source       = hash_source_factory_(element);
  std::size_t bits_checked = 0u;
  for (std::size_t const hash : source)
  {
    ++bits_checked;
    if (bits_.bit(hash % bits_.size()) == 0u)
    {
      return {false, bits_checked};
    }
  }

  return {true, bits_checked};
}

void BasicBloomFilter::Add(fetch::byte_array::ConstByteArray const &element)
{
  auto const source = hash_source_factory_(element);
  for (std::size_t const hash : source)
  {
    auto const bit_index = hash % bits_.size();

    bits_.set(bit_index, 1u);
  }
}

void BasicBloomFilter::Reset()
{
  bits_.SetAllZero();
}

}  // namespace fetch
