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

#include "bloom_filter/bloom_filter.hpp"

#include <cstddef>
#include <istream>
#include <memory>
#include <ostream>
#include <utility>

namespace fetch {

namespace byte_array {
class ConstByteArray;
}

class ProgressiveBloomFilter
{
public:
  ProgressiveBloomFilter()                               = default;
  ProgressiveBloomFilter(ProgressiveBloomFilter const &) = delete;
  ProgressiveBloomFilter(ProgressiveBloomFilter &&)      = delete;
  ~ProgressiveBloomFilter()                              = default;

  ProgressiveBloomFilter &operator=(ProgressiveBloomFilter const &) = delete;
  ProgressiveBloomFilter &operator=(ProgressiveBloomFilter &&) = default;

  std::pair<bool, std::size_t> Match(fetch::byte_array::ConstByteArray const &element,
                                     std::size_t                              element_index) const;
  void Add(fetch::byte_array::ConstByteArray const &element, std::size_t element_index,
           std::size_t current_head_index);

  bool IsInCurrentRange(std::size_t index) const;
  void Reset();

private:
  std::size_t                       current_min_index_{};
  std::size_t                       overlap_{20000};
  std::unique_ptr<BasicBloomFilter> filter1_{std::make_unique<BasicBloomFilter>()};
  std::unique_ptr<BasicBloomFilter> filter2_{std::make_unique<BasicBloomFilter>()};

  template <typename, typename>
  friend struct fetch::serializers::MapSerializer;
};

namespace serializers {

template <typename D>
struct MapSerializer<ProgressiveBloomFilter, D>
{
public:
  using Type       = ProgressiveBloomFilter;
  using DriverType = D;

  static const uint8_t MIN_INDEX = 1;
  static const uint8_t OVERLAP   = 2;
  static const uint8_t FILTER1   = 3;
  static const uint8_t FILTER2   = 4;

  template <typename T>
  static void Serialize(T &map_constructor, Type const &filter)
  {
    auto map = map_constructor(4);
    map.Append(MIN_INDEX, static_cast<uint64_t>(filter.current_min_index_));
    map.Append(OVERLAP, static_cast<uint64_t>(filter.overlap_));
    map.Append(FILTER1, *filter.filter1_);
    map.Append(FILTER2, *filter.filter2_);
  }

  template <typename T>
  static void Deserialize(T &map, Type &filter)
  {
    uint64_t &min_index = filter.current_min_index_;
    uint64_t &overlap   = filter.overlap_;

    map.ExpectKeyGetValue(MIN_INDEX, min_index);
    map.ExpectKeyGetValue(OVERLAP, overlap);
    map.ExpectKeyGetValue(FILTER1, *filter.filter1_);
    map.ExpectKeyGetValue(FILTER2, *filter.filter2_);
  }
};

}  // namespace serializers
}  // namespace fetch
