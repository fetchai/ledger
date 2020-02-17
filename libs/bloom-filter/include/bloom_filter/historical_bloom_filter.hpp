#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/main_serializer_definition.hpp"
#include "storage/single_object_store.hpp"
#include "storage/fixed_size_journal.hpp"

#include <cstdint>
#include <unordered_map>

namespace fetch {
namespace bloom {

class HistoricalBloomFilter
{
public:
  using ConstByteArray      = byte_array::ConstByteArray;
  using BasicBloomFilterPtr = std::unique_ptr<BasicBloomFilter>;

  struct CacheEntry
  {
    BasicBloomFilterPtr filter{};
    bool                dirty{false};

    bool Match(ConstByteArray const &element) const;
  };

  enum class Mode
  {
    NEW_DATABASE,
    LOAD_DATABASE
  };

  // Construction / Destruction
  HistoricalBloomFilter(Mode mode, char const *store_path, char const *metadata_path,
                        uint64_t window_size, std::size_t max_num_cached_buckets);
  HistoricalBloomFilter(HistoricalBloomFilter const &) = delete;
  HistoricalBloomFilter(HistoricalBloomFilter &&)      = delete;
  ~HistoricalBloomFilter();

  bool Add(ConstByteArray const &element, uint64_t index);
  bool Match(ConstByteArray const &element, uint64_t minimum_index, uint64_t maximum_index) const;
  std::size_t TrimCache();
  void        Flush();
  void        Reset();

  uint64_t last_flushed_bucket() const;
  uint64_t window_size() const;

  // Operators
  HistoricalBloomFilter &operator=(HistoricalBloomFilter const &) = delete;
  HistoricalBloomFilter &operator=(HistoricalBloomFilter &&) = delete;

private:
  using Cache         = std::unordered_map<uint64_t, CacheEntry>;
  using MetadataStore = fetch::storage::SingleObjectStore;
  using FilterStore   = fetch::storage::FixedSizeJournalFile;

  uint64_t ToBucket(uint64_t index) const;
  void     AddToBucket(ConstByteArray const &element, uint64_t bucket);
  bool     MatchInBucket(ConstByteArray const &element, uint64_t bucket) const;
  bool     MatchInStore(ConstByteArray const &element, uint64_t bucket) const;

  bool LookupBucketFromStore(uint64_t bucket, CacheEntry &entry) const;
  bool SaveBucketToStore(uint64_t bucket, CacheEntry const &entry);
  void UpdateMetadata();

  std::string          store_filename_{};
  MetadataStore        metadata_{};
  uint64_t             window_size_{0};
  uint64_t             heaviest_persisted_bucket_{0};
  std::size_t          max_num_cached_buckets_{1};
  Cache                cache_{};
  FilterStore          store_;
};

inline uint64_t HistoricalBloomFilter::last_flushed_bucket() const
{
  return heaviest_persisted_bucket_;
}

inline uint64_t HistoricalBloomFilter::window_size() const
{
  return window_size_;
}

}  // namespace bloom
}  // namespace fetch
