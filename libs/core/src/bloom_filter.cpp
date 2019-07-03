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

namespace fetch {

HashSourceFactory::HashSourceFactory(fetch::HashSourceFactory::Functions hash_functions)
  : hash_functions_(std::move(hash_functions))
{}

HashSourceFactory::~HashSourceFactory() = default;

HashSource HashSourceFactory::operator()(Bytes const &element) const
{
  return HashSource(this->hash_functions_, element);
}

HashSource::HashSource(HashSourceFactory::Functions const &hash_functions,
                       HashSourceFactory::Bytes const &    input)
  : data_{}
{
  // TODO(WK): lazily evaluate hashes
  for (auto const &fn : hash_functions)
  {
    auto const hashes = fn(input);
    data_.insert(data_.cend(), hashes.cbegin(), hashes.cend());
  }
}

HashSource::~HashSource() = default;

HashSourceIterator HashSource::begin() const
{
  return HashSourceIterator{this, 0u};
}

HashSourceIterator HashSource::end() const
{
  return HashSourceIterator{this, data_.size()};
}

HashSourceIterator HashSource::cbegin() const
{
  return HashSourceIterator{this, 0u};
}

HashSourceIterator HashSource::cend() const
{
  return HashSourceIterator{this, data_.size()};
}

HashSourceIterator::~HashSourceIterator() = default;

bool HashSourceIterator::operator!=(HashSourceIterator const &other) const
{
  return !(operator==(other));
}

bool HashSourceIterator::operator==(HashSourceIterator const &other) const
{
  return source_ == other.source_ && hash_index_ == other.hash_index_;
}

HashSourceIterator const HashSourceIterator::operator++(int)
{
  HashSourceIterator temp{source_, hash_index_};

  operator++();

  return temp;
}

HashSourceIterator &HashSourceIterator::operator++()
{
  ++hash_index_;

  return *this;
}

std::size_t HashSourceIterator::operator*() const
{
  return source_->data_[hash_index_];
}

HashSourceIterator::HashSourceIterator(HashSource const *source, std::size_t index)
  : hash_index_{index}
  , source_{source}
{}

namespace {

class OpenSslHasher  //???move to crypto, make reusable
{
public:
  enum class Type
  {
    MD5,
    SHA_2_512,
    SHA_3_512
  };

  explicit OpenSslHasher(Type type)
    : type_{type}
    , ctx_{EVP_MD_CTX_create()}
  {
    EVP_MD_CTX_init(ctx_);
  }

  ~OpenSslHasher()
  {
    EVP_MD_CTX_destroy(ctx_);
  }

  std::vector<std::size_t> operator()(HashSourceFactory::Bytes const &input) const
  {
    init_digest_operation_();

    const auto size_in_bytes = static_cast<std::size_t>(EVP_MD_CTX_size(ctx_));

    std::vector<std::size_t> output(size_in_bytes / sizeof(std::size_t));
    EVP_DigestUpdate(ctx_, input.pointer(), input.size());

    EVP_DigestFinal_ex(ctx_, reinterpret_cast<uint8_t *>(output.data()), nullptr);

    return output;
  }

private:
  void init_digest_operation_() const
  {
    EVP_MD const *openssl_type = nullptr;
    switch (type_)
    {
    case Type::MD5:
      openssl_type = EVP_sha512();
      break;
    case Type::SHA_2_512:
      openssl_type = EVP_sha512();
      break;
    case Type::SHA_3_512:
      openssl_type = EVP_sha3_512();
      break;
    }

    assert(openssl_type);
    EVP_DigestInit_ex(ctx_, openssl_type, nullptr);
  }

  Type const        type_;
  EVP_MD_CTX *const ctx_;
};

std::vector<std::size_t> raw_data(HashSourceFactory::Bytes const &input)
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

std::vector<std::size_t> md5(HashSourceFactory::Bytes const &input)
{
  OpenSslHasher hasher(OpenSslHasher::Type::MD5);

  return hasher(input);
}

std::vector<std::size_t> sha_2_512(HashSourceFactory::Bytes const &input)
{
  OpenSslHasher hasher(OpenSslHasher::Type::SHA_2_512);

  return hasher(input);
}

std::vector<std::size_t> sha_3_512(HashSourceFactory::Bytes const &input)
{
  OpenSslHasher hasher(OpenSslHasher::Type::SHA_3_512);

  return hasher(input);
}

std::vector<std::size_t> fnv(HashSourceFactory::Bytes const &input)
{
  crypto::FNV hasher;
  hasher.Reset();
  hasher.Update(reinterpret_cast<std::uint8_t const *>(input.pointer()), input.size());

  const auto               size_in_bytes = hasher.GetSizeInBytes();
  std::vector<std::size_t> output(size_in_bytes / sizeof(std::size_t));

  hasher.Final(reinterpret_cast<uint8_t *>(output.data()), output.size() * sizeof(std::size_t));

  return output;
}

}  // namespace

BloomFilter::Functions const default_hash_functions = {raw_data, md5, sha_2_512, sha_3_512, fnv};

constexpr std::size_t const INITIAL_SIZE_IN_BITS = 8 * 1024 * 1024;

BloomFilter::~BloomFilter() = default;

BloomFilter::BloomFilter()
  : bits_(INITIAL_SIZE_IN_BITS)
  , hash_source_factory_(default_hash_functions)
{}

BloomFilter::BloomFilter(Functions const &functions)
  : bits_(INITIAL_SIZE_IN_BITS)
  , hash_source_factory_(functions)
{}

bool BloomFilter::Match(Bytes const &element) const
{
  auto const source = hash_source_factory_(element);
  for (std::size_t const hash : source)
  {
    if (!bits_.bit(hash % bits_.size()))
    {
      return false;
    }
  }

  return true;
}

void BloomFilter::Add(Bytes const &element)
{
  auto const source = hash_source_factory_(element);
  for (std::size_t const hash : source)
  {
    bits_.set(hash % bits_.size(), 1u);
  }
}

}  // namespace fetch
