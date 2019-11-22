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
 *
 * The factory must be kept alive while its HashSource instances remain
 * in use.
 */
class HashSourceFactory
{
private:
  using Function =
      std::function<std::vector<std::size_t>(fetch::byte_array::ConstByteArray const &)>;

public:
  using Functions = std::vector<Function>;

  /*
   * Construct a factory with the given set of hash functions.
   */
  explicit HashSourceFactory(Functions hash_functions);
  HashSourceFactory()                          = delete;
  HashSourceFactory(HashSourceFactory const &) = delete;
  HashSourceFactory(HashSourceFactory &&)      = default;
  ~HashSourceFactory()                         = default;

  HashSourceFactory &operator=(HashSourceFactory const &) = delete;
  HashSourceFactory &operator=(HashSourceFactory &&) = default;

  /*
   * Create a HashSource which, when iterated, will pass the input parameter
   * to the hash functions in sequence.
   */
  HashSource operator()(fetch::byte_array::ConstByteArray const &element) const;

private:
  Functions hash_functions_;
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
  using Hashes = std::vector<std::size_t>;

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

    /*
     * Compare iterators for equality. Returns true if the iterators were
     * generated by the same HashSource and are pointing at the same hash;
     * false otherwise;
     */
    bool operator==(HashSourceIterator const &other) const;

    /*
     * Check if iterators are different. Equivalent to !operator==(other)
     */
    bool operator!=(HashSourceIterator const &other) const;

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
  HashSource(HashSourceFactory::Functions const &     hash_functions,
             fetch::byte_array::ConstByteArray const &input);

  /*
   * Used by instances of HashSourceIterator to retrieve a hash at the given index.
   */
  std::size_t getHash(std::size_t index) const;

  Hashes data_;

  friend class HashSourceFactory;
};

}  // namespace internal

class BasicBloomFilter
{
public:
  using Functions = internal::HashSourceFactory::Functions;

  /*
   * Construct a Bloom filter with a default set of hash functions
   */
  BasicBloomFilter();

  /*
   * Construct a Bloom filter with the given set of hash functions
   */
  explicit BasicBloomFilter(Functions const &functions);
  BasicBloomFilter(BasicBloomFilter const &) = delete;
  BasicBloomFilter(BasicBloomFilter &&)      = delete;
  ~BasicBloomFilter()                        = default;

  BasicBloomFilter &operator=(BasicBloomFilter const &) = delete;
  BasicBloomFilter &operator=(BasicBloomFilter &&) = default;

  /*
   * Check if the argument matches the Bloom filter. Returns a pair of
   * a Boolean (false if the element had never been added; true if the
   * argument had been added or is a false positive) and a positive integer
   * which indicates how many bits had to be checked before the function
   * returned. The latter number will increase as the filter's performance
   * degrades.
   */
  std::pair<bool, std::size_t> Match(fetch::byte_array::ConstByteArray const &element) const;

  /*
   * Set the bits of the Bloom filter corresponding to the argument
   */
  void Add(fetch::byte_array::ConstByteArray const &element);

  /*
   * Empty the Bloom filter (set all bits to zero). Preserves filter size and hash set.
   */
  void Reset();

  BitVector *getSerialisationData()
  {
    return &bits_;
  }

  BitVector const *getSerialisationData() const
  {
    return &bits_;
  }

private:
  BitVector                   bits_;
  internal::HashSourceFactory hash_source_factory_;
};

namespace serializers {

template <typename D>
struct MapSerializer<BasicBloomFilter, D>
{
public:
  using Type       = BasicBloomFilter;
  using DriverType = D;

  static const uint8_t BITS = 1;

  template <typename T>
  static void Serialize(T &map_constructor, Type const &filter)
  {
    auto const bits = filter.getSerialisationData();

    auto map = map_constructor(1);
    map.Append(BITS, *bits);
  }

  template <typename T>
  static void Deserialize(T &map, Type &filter)
  {
    auto bits = filter.getSerialisationData();

    map.ExpectKeyGetValue(BITS, *bits);
  }
};

}  // namespace serializers
}  // namespace fetch
