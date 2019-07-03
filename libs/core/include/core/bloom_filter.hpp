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

class HashSource;

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

class HashSource
{
public:
  ~HashSource();

  HashSourceIterator begin() const;
  HashSourceIterator end() const;
  HashSourceIterator cbegin() const;
  HashSourceIterator cend() const;

private:
  HashSource(HashSourceFactory::Functions const &, HashSourceFactory::Bytes const &);

  mutable std::vector<std::size_t> data_;

  friend class HashSourceFactory;
  friend class HashSourceIterator;
};

class BloomFilter
{
public:
  using Bytes     = HashSourceFactory::Bytes;
  using Function  = HashSourceFactory::Function;
  using Functions = HashSourceFactory::Functions;

  BloomFilter();
  explicit BloomFilter(Functions const &fns);
  ~BloomFilter();

  BloomFilter(BloomFilter const &) = delete;
  BloomFilter(BloomFilter &&)      = delete;
  BloomFilter &operator=(BloomFilter const &) = delete;
  BloomFilter &operator=(BloomFilter &&) = delete;

  bool Match(Bytes const &element) const;
  void Add(Bytes const &element);

public:
  BitVector               bits_;
  HashSourceFactory const hash_source_factory_;
};

}  // namespace fetch
