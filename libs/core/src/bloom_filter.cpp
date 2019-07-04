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

#include "core/bitvector.hpp"
#include "core/bloom_filter.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"

#include <openssl/evp.h>

#include <cassert>
#include <cstddef>
#include <functional>
#include <vector>

// Aim for 1 false positive per this many positive queries
constexpr std::size_t const INVERSE_TARGET_FALSE_POSITIVE_RATE = 100000u;

// No point in evaluating the filter's quality until this many positive queries
// have been executed
constexpr std::size_t const MEANINGFUL_STATS_THRESHOLD = 5u * INVERSE_TARGET_FALSE_POSITIVE_RATE;

constexpr std::size_t const INITIAL_SIZE_IN_BITS = 8 * 10 * 1024 * 1024;

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
  : data_{}
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

class OpenSslHasher
{
public:
  enum class Type
  {
    MD5,
    SHA1,
    SHA2_512
  };

  explicit OpenSslHasher(Type type)
    : openssl_type_{to_openssl_type(type)}
    , ctx_{EVP_MD_CTX_create()}
  {
    EVP_MD_CTX_init(ctx_);
  }

  ~OpenSslHasher()
  {
    EVP_MD_CTX_destroy(ctx_);
  }

  void reset() const
  {
    EVP_DigestInit_ex(ctx_, openssl_type_, nullptr);
  }

  std::vector<std::size_t> operator()(fetch::byte_array::ConstByteArray const &input) const
  {
    const auto size_in_bytes = static_cast<std::size_t>(EVP_MD_CTX_size(ctx_));

    std::vector<std::size_t> output((size_in_bytes + sizeof(std::size_t) - 1) /
                                    sizeof(std::size_t));
    EVP_DigestUpdate(ctx_, input.pointer(), input.size());

    EVP_DigestFinal_ex(ctx_, reinterpret_cast<uint8_t *>(output.data()), nullptr);

    return output;
  }

private:
  EVP_MD const *to_openssl_type(Type const type) const
  {
    EVP_MD const *openssl_type = nullptr;
    switch (type)
    {
    case Type::MD5:
      openssl_type = EVP_sha512();
      break;
    case Type::SHA1:
      openssl_type = EVP_sha1();
      break;
    case Type::SHA2_512:
      openssl_type = EVP_sha512();
      break;
    }

    assert(openssl_type);

    return openssl_type;
  }

  EVP_MD const *const openssl_type_;
  EVP_MD_CTX *const   ctx_;
};

std::vector<std::size_t> raw_data(fetch::byte_array::ConstByteArray const &input)
{
  auto start = reinterpret_cast<std::size_t const *>(input.pointer());

  const auto size_in_bytes = input.size();

  std::vector<std::size_t> output((size_in_bytes + sizeof(std::size_t) - 1) / sizeof(std::size_t));

  for (std::size_t i = 0; i < output.size(); ++i)
  {
    output[i] = *(start + i);
  }

  return output;
}

HashSource::Hashes md5(fetch::byte_array::ConstByteArray const &input)
{
  static OpenSslHasher hasher(OpenSslHasher::Type::MD5);
  hasher.reset();

  return hasher(input);
}

HashSource::Hashes sha1(fetch::byte_array::ConstByteArray const &input)
{
  static OpenSslHasher hasher(OpenSslHasher::Type::SHA1);
  hasher.reset();

  return hasher(input);
}

HashSource::Hashes sha2_512(fetch::byte_array::ConstByteArray const &input)
{
  static OpenSslHasher hasher(OpenSslHasher::Type::SHA2_512);
  hasher.reset();

  return hasher(input);
}

HashSource::Hashes fnv(fetch::byte_array::ConstByteArray const &input)
{
  static crypto::FNV hasher;
  hasher.Reset();

  hasher.Update(reinterpret_cast<std::uint8_t const *>(input.pointer()), input.size());

  const auto         size_in_bytes = hasher.GetSizeInBytes();
  HashSource::Hashes output(size_in_bytes / sizeof(std::size_t));

  hasher.Final(reinterpret_cast<uint8_t *>(output.data()), output.size() * sizeof(std::size_t));

  return output;
}

}  // namespace

}  // namespace internal

BasicBloomFilter::Functions const default_hash_functions{
    internal::raw_data, internal::md5, internal::sha1, internal::sha2_512, internal::fnv};

BasicBloomFilter::BasicBloomFilter()
  : bits_(INITIAL_SIZE_IN_BITS)
  , hash_source_factory_(default_hash_functions)
{}

BasicBloomFilter::BasicBloomFilter(Functions const &functions)
  : bits_(INITIAL_SIZE_IN_BITS)
  , hash_source_factory_(functions)
{}

bool BasicBloomFilter::Match(fetch::byte_array::ConstByteArray const &element)
{
  auto const source = hash_source_factory_(element);
  for (std::size_t const hash : source)
  {
    if (!bits_.bit(hash % bits_.size()))
    {
      return false;
    }
  }

  ++positive_count_;
  return true;
}

void BasicBloomFilter::Add(fetch::byte_array::ConstByteArray const &element)
{
  bool is_new_entry = false;

  auto const source = hash_source_factory_(element);
  for (std::size_t const hash : source)
  {
    auto const bit_index = hash % bits_.size();

    if (!bits_.bit(bit_index))
    {
      is_new_entry = true;
    }

    bits_.set(bit_index, 1u);
  }

  if (is_new_entry)
  {
    ++entry_count_;
  }
}

bool BasicBloomFilter::ReportFalsePositives(std::size_t count)
{
  false_positive_count_ += count;
  if (positive_count_ > MEANINGFUL_STATS_THRESHOLD)
  {
    return static_cast<std::size_t>(positive_count_ / false_positive_count_) >
           INVERSE_TARGET_FALSE_POSITIVE_RATE;
  }

  return false;
}

bool NullBloomFilter::Match(fetch::byte_array::ConstByteArray const &)
{
  return true;
}

void NullBloomFilter::Add(fetch::byte_array::ConstByteArray const &)
{}

bool NullBloomFilter::ReportFalsePositives(std::size_t)
{
  return true;
}

}  // namespace fetch
