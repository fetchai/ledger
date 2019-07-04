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
 */
class HashSourceFactory
{
public:
  using Bytes     = fetch::byte_array::ConstByteArray;
  using Function  = std::function<std::vector<std::size_t>(Bytes const &)>;
  using Functions = std::vector<Function>;

  explicit HashSourceFactory(Functions);
  ~HashSourceFactory();

  HashSource operator()(Bytes const &) const;

private:
  Functions const hash_functions_;
};

/*
 * Represents a sequential application of a HashSourceFactory's hash
 * functions to a byte array. Outwardly it may be treated as an immutable,
 * iterable collection of std::size_t.
 *
 * Not thread-safe.
 */
class HashSource
{
public:
  ~HashSource();

  class HashSourceIterator
  {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = std::size_t const;
    using difference_type   = std::ptrdiff_t;
    using pointer           = std::size_t const *;
    using reference         = std::size_t const &;

    ~HashSourceIterator();

    bool operator!=(HashSourceIterator const &other) const;
    bool operator==(HashSourceIterator const &other) const;

    HashSourceIterator const operator++(int);
    HashSourceIterator &     operator++();

    std::size_t operator*() const;

  private:
    explicit HashSourceIterator(HashSource const *source, std::size_t index);

    std::size_t       hash_index_;
    HashSource const *source_;

    friend class HashSource;
  };

  HashSourceIterator begin() const;
  HashSourceIterator end() const;
  HashSourceIterator cbegin() const;
  HashSourceIterator cend() const;

private:
  HashSource(HashSourceFactory::Functions const &, HashSourceFactory::Bytes const &);

  std::vector<std::size_t> data_;

  friend class HashSourceFactory;
};

}  // namespace internal

class BloomFilter
{
public:
  using Bytes     = internal::HashSourceFactory::Bytes;
  using Function  = internal::HashSourceFactory::Function;
  using Functions = internal::HashSourceFactory::Functions;

  enum class Type
  {
    DUMMY,
    BASIC
  };

  static std::unique_ptr<BloomFilter> create(Type);

  virtual ~BloomFilter() = default;

  /*
   * Query the Bloom filter for a given entry.
   *
   * Returns false if the entry is definitely absent; true otherwise.
   */
  virtual bool Match(Bytes const &) = 0;

  /*
   * Add a new entry to the filter.
   */
  virtual void Add(Bytes const &) = 0;

  /*
   * Clients may use this to report how many false positives they identified.
   * This information is used internally by the filter to keep track of the
   * false positive rate.
   *
   * Return false if the filter's measured false positive rate exceeds its
   * target value and rebuilding the filter may be advisable; true otherwise.
   */
  virtual bool ReportFalsePositives(std::size_t) = 0;
};

class BasicBloomFilter : public BloomFilter
{
public:
  BasicBloomFilter();
  explicit BasicBloomFilter(Functions const &);
  ~BasicBloomFilter() override;

  BasicBloomFilter(BasicBloomFilter const &) = delete;
  BasicBloomFilter(BasicBloomFilter &&)      = delete;
  BasicBloomFilter &operator=(BasicBloomFilter const &) = delete;
  BasicBloomFilter &operator=(BasicBloomFilter &&) = delete;

  bool Match(Bytes const &) override;
  void Add(Bytes const &) override;
  bool ReportFalsePositives(std::size_t) override;

public:
  BitVector                         bits_;
  internal::HashSourceFactory const hash_source_factory_;
  std::size_t                       entry_count_;
  std::size_t                       positive_count_;
  std::size_t                       false_positive_count_;
};

/*
 * A fake Bloom filter which holds no data and treats any query as positive.
 */
class DummyBloomFilter : public BloomFilter
{
public:
  DummyBloomFilter();
  ~DummyBloomFilter() override;

  bool Match(Bytes const &) override;
  void Add(Bytes const &) override;
  bool ReportFalsePositives(std::size_t) override;
};

}  // namespace fetch
