#pragma once
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
#include "core/bloom_filter_interface.hpp"

#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>

namespace fetch {

namespace byte_array {
class ConstByteArray;
}

namespace internal {

class HashSource;

/*
 * An ordered collection of hash functions for generating pseudorandom
 * std::size_t indices for the Bloom filter. To apply the functions,
 * to an input, invoke the factory's operator() and use the resulting
 * HashSource.
 *
 * The factory must be kept alive while its HashSource instances remain
 * in use.
 */
class HashSourceFactory
{
public:
  using Bytes     = fetch::byte_array::ConstByteArray;
  using Function  = std::function<std::vector<std::size_t>(Bytes const &)>;
  using Functions = std::vector<Function>;

  explicit HashSourceFactory(Functions);
  HashSourceFactory()                          = delete;
  HashSourceFactory(HashSourceFactory const &) = delete;
  HashSourceFactory(HashSourceFactory &&)      = delete;
  ~HashSourceFactory()                         = default;

  HashSourceFactory &operator=(HashSourceFactory const &) = delete;
  HashSourceFactory &operator=(HashSourceFactory &&) = delete;

  HashSource operator()(Bytes const &) const;

private:
  Functions const hash_functions_;
};

/*
 * Represents a sequential application of a HashSourceFactory's hash functions
 * to a byte array. Outwardly it may be treated as an immutable, iterable
 * collection of std::size_t.
 *
 * Not thread-safe. Not safe to use after the parent HashSourceFactory
 * had been destroyed.
 */
class HashSource
{
public:
  HashSource()                       = delete;
  HashSource(HashSource const &)     = delete;
  HashSource(HashSource &&) noexcept = default;
  ~HashSource()                      = default;

  HashSource &operator=(HashSource const &) = delete;
  HashSource &operator=(HashSource &&) noexcept = default;

  class HashSourceIterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = std::size_t const;
    using difference_type   = std::ptrdiff_t;
    using pointer           = std::size_t const *;
    using reference         = std::size_t const &;

    ~HashSourceIterator()                              = default;
    HashSourceIterator(HashSourceIterator const &)     = default;
    HashSourceIterator(HashSourceIterator &&) noexcept = default;
    HashSourceIterator()                               = delete;

    HashSourceIterator &operator=(HashSourceIterator const &) = default;
    HashSourceIterator &operator=(HashSourceIterator &&) noexcept = default;

    bool operator!=(HashSourceIterator const &other) const;
    bool operator==(HashSourceIterator const &other) const;

    HashSourceIterator &operator++();

    std::size_t operator*() const;

  private:
    explicit HashSourceIterator(HashSource const *source, std::size_t index);

    std::size_t       hash_index_{};
    HashSource const *source_{};

    friend class HashSource;
  };

  HashSourceIterator begin() const;
  HashSourceIterator end() const;
  HashSourceIterator cbegin() const;
  HashSourceIterator cend() const;

private:
  HashSource(HashSourceFactory::Functions const &, HashSourceFactory::Bytes const &);

  std::size_t getHash(std::size_t index) const;

  std::vector<std::size_t> data_;

  friend class HashSourceFactory;
};

}  // namespace internal

class BasicBloomFilter : public BloomFilterInterface
{
public:
  using Functions = internal::HashSourceFactory::Functions;

  BasicBloomFilter();
  explicit BasicBloomFilter(Functions const &);
  BasicBloomFilter(BasicBloomFilter const &) = delete;
  BasicBloomFilter(BasicBloomFilter &&)      = delete;
  ~BasicBloomFilter() override               = default;

  BasicBloomFilter &operator=(BasicBloomFilter const &) = delete;
  BasicBloomFilter &operator=(BasicBloomFilter &&) = delete;

  bool Match(Bytes const &) override;
  void Add(Bytes const &) override;
  bool ReportFalsePositives(std::size_t) override;

public:
  BitVector                         bits_;
  internal::HashSourceFactory const hash_source_factory_;
  std::size_t                       entry_count_{};
  std::size_t                       positive_count_{};
  std::size_t                       false_positive_count_{};
};

/*
 * A fake Bloom filter which holds no data and treats any query as positive.
 */
class NullBloomFilter : public BloomFilterInterface
{
public:
  NullBloomFilter()                        = default;
  NullBloomFilter(NullBloomFilter const &) = delete;
  NullBloomFilter(NullBloomFilter &&)      = delete;
  ~NullBloomFilter() override              = default;

  NullBloomFilter &operator=(NullBloomFilter const &) = delete;
  NullBloomFilter &operator=(NullBloomFilter &&) = delete;

  bool Match(Bytes const &) override;
  void Add(Bytes const &) override;
  bool ReportFalsePositives(std::size_t) override;
};

}  // namespace fetch
