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

#include "bloom_filter/historical_bloom_filter.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/containers/is_in.hpp"
#include "core/serializers/main_serializer.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

#include <set>

namespace fetch {
namespace serializers {

template <typename D>
struct BinarySerializer<bloom::HistoricalBloomFilter::CacheEntry, D>
{
public:
  using Type       = bloom::HistoricalBloomFilter::CacheEntry;
  using DriverType = D;
  using EnumType   = uint8_t;

  static const uint8_t TYPE = 1;

  template <typename T>
  static void Serialize(T &bin_constructor, Type const &cache)
  {
    BitVector const *bit_vector{nullptr};
    if (cache.filter)
    {
      bit_vector = &cache.filter->underlying_bit_vector();
    }

    // calculate the required size of disk for the bit vector
    std::size_t const required_size =
        (bit_vector) ? bit_vector->blocks() * sizeof(BitVector::Block) : 0;

    // make the message pack binary builder and write out the data
    auto builder = bin_constructor(required_size);

    auto const *buffer = reinterpret_cast<uint8_t const *>(bit_vector->data().pointer());
    builder.Write(buffer, required_size);
  }

  template <typename T>
  static void Deserialize(T &bin, Type &msg)
  {
    std::size_t const vector_size = bin.size();

    if (vector_size == 0)
    {
      // create a fresh cache entry
      msg.filter.reset();
      msg.dirty = false;
      return;
    }

    if ((vector_size % sizeof(BitVector::Block)) != 0)
    {
      throw std::runtime_error("Vector oddly sizes, contains partial blocks");
    }

    // read the complete bit vector into memory
    BitVector vector{vector_size * 8u};
    auto *    buffer = reinterpret_cast<uint8_t *>(vector.data().pointer());
    bin.Read(buffer, vector_size);

    // finally build the cache entry
    msg.filter = std::make_unique<BasicBloomFilter>(vector);
    msg.dirty  = false;
  }
};

}  // namespace serializers

namespace bloom {
namespace {

using telemetry::Registry;

struct BloomFilterMetadata
{
  uint64_t window_size;
  uint64_t last_bucket;
};

static_assert(meta::IsPOD<BloomFilterMetadata>, "Metadata must be POD");

constexpr char const *LOGGING_NAME = "HBloomFilter";

constexpr std::size_t BLOOM_FILTER_SIZE = 160 * 8 * 1024 * 1024;

// this number is vague adjustment to allow room for current and future serialisation over head
// on the filter. This is, therefore is not a strictly derived value
constexpr uint64_t STORAGE_SECTOR_SIZE = BLOOM_FILTER_SIZE + (BLOOM_FILTER_SIZE >> 3);  // 112.5 %

/**
 * Generate a order array of the keys of the input map
 *
 * @tparam T The type of the mapped object
 * @param values The map of value to process
 * @return The array of keys
 */
template <typename T>
std::vector<uint64_t> GetOrderedKeys(std::unordered_map<uint64_t, T> const &values)
{
  std::vector<uint64_t> keys{};
  keys.reserve(values.size());

  // populate the key array
  for (auto const &element : values)
  {
    keys.push_back(element.first);
  }

  // sort the values
  std::sort(keys.begin(), keys.end());

  return keys;
}

}  // namespace

/**
 * Constructs a historical bloom filter with desired window size
 *
 * @param mode The configured database mode for the bloom filter
 * @param store_path The path to the main database file
 * @param metadata_path The path to the metadata file
 * @param window_size The size of the window
 * @param max_num_cached_buckets The maximum number of cached buckets
 */
HistoricalBloomFilter::HistoricalBloomFilter(Mode mode, char const *store_path,
                                             char const *metadata_path, uint64_t window_size,
                                             std::size_t max_num_cached_buckets)
  : store_filename_{store_path}
  , window_size_{window_size}
  , max_num_cached_buckets_{max_num_cached_buckets}
  , store_{STORAGE_SECTOR_SIZE}
  , total_additions_{Registry::Instance().CreateCounter(
        "ledger_hbloom_additions_total", "The total number of entries added to the bloom filter")}
  , total_positive_matches_{Registry::Instance().CreateCounter(
        "ledger_hbloom_positive_matches_total",
        "The total number of positive matches items in the bloom filter")}
  , total_negative_matches_{Registry::Instance().CreateCounter(
        "ledger_hbloom_negative_matches_total",
        "The total number of negative matches items in the bloom filter")}
  , total_save_failures_{Registry::Instance().CreateCounter(
        "ledger_hbloom_save_failures_total", "The total number of bucket save failures")}
  , num_pages_in_memory_{Registry::Instance().CreateGauge<uint64_t>(
        "ledger_hbloom_cached_pages", "The total number of bloom filter entries in memory")}
  , last_bloom_filter_level_{Registry::Instance().CreateGauge<uint64_t>(
        "ledger_hbloom_last_fill_level",
        "The last number of bits that was checked to find a match")}
  , max_bloom_filter_level_{Registry::Instance().CreateGauge<uint64_t>(
        "ledger_hbloom_max_fill_level",
        "The current maximum number of bits that have been searched to find a positive result")}
{
  metadata_.Load(metadata_path);

  // create the default metadata for the bloom filter
  BloomFilterMetadata metadata{};
  metadata.window_size = window_size;
  metadata.last_bucket = 0;

  switch (mode)
  {
  case Mode::NEW_DATABASE:
    if (!store_.New(store_filename_))
    {
      throw std::runtime_error("Unable to create database file");
    }

    // Overwrite metadata in the file
    try
    {
      metadata_.Set(metadata);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to write metadata to file: ", ex.what());
    }

    break;
  case Mode::LOAD_DATABASE:
    if (!store_.Load(store_filename_))
    {
      throw std::runtime_error("Unable to load or create existing database file");
    }

    // Check metadata can be retrieved and is correct
    try
    {
      metadata_.Get(metadata);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to read metadata from file: ", ex.what());
    }

    if (metadata.window_size != window_size)
    {
      throw std::runtime_error("The window size is not configured to match previous version");
    }

    // update the last flushed bucket
    heaviest_persisted_bucket_ = metadata.last_bucket;
    break;
  }
}

/**
 * Destructor of the bloom filter, make sure all changes are flushed to disk
 */
HistoricalBloomFilter::~HistoricalBloomFilter()
{
  Flush();
}

/**
 * Adds a element into the bloom filter
 *
 * @param element The element value to be added
 * @param index The index value associated with this index
 * @return true if successful, otherwise false
 */
bool HistoricalBloomFilter::Add(ConstByteArray const &element, uint64_t index)
{
  uint64_t const bucket = ToBucket(index);

  // if the bucket is not in the cache then we need to load it into the cache
  if (!core::IsIn(cache_, bucket))
  {
    CacheEntry entry;
    if (LookupBucketFromStore(bucket, entry))
    {
      if (!cache_.emplace(bucket, std::move(entry)).second)
      {
        // for some reason we could not insert the entry into the bucket - something internal has
        // gone wrong
        return false;
      }
    }
  }

  // finally add the element to the bucket
  AddToBucket(element, bucket);

  total_additions_->increment();

  return true;
}

/**
 * Matches a element given an upper and lower bound to be index value associated with it
 *
 * @param element The element to be looked up
 * @param minimum_index The lower bound on the elements index value
 * @param maximum_index The upper bound of the elements index value
 * @return false if the element is not a match, otherwise true
 */
bool HistoricalBloomFilter::Match(ConstByteArray const &element, uint64_t minimum_index,
                                  uint64_t maximum_index) const
{
  bool is_match{false};

  // iterate over all the buckets to which this element can apply
  uint64_t const first_bucket = ToBucket(minimum_index);
  uint64_t const last_bucket  = ToBucket(maximum_index);
  uint64_t const delta_bucket = (last_bucket - first_bucket) + 1;

  for (uint64_t idx = 0; idx < delta_bucket; ++idx)
  {
    // search backwards
    uint64_t const bucket = last_bucket - idx;

    auto const result = MatchInBucket(element, bucket);
    if (result.match)
    {
      is_match = true;

      total_positive_matches_->increment();
      max_bloom_filter_level_->max(result.bits_checked);
      last_bloom_filter_level_->set(result.bits_checked);
      break;
    }

    total_negative_matches_->increment();
  }

  return is_match;
}

/**
 * Trim the historical bloom filter cache
 */
std::size_t HistoricalBloomFilter::TrimCache()
{
  std::size_t pages_flushed{0};
  std::size_t pages_updated{0};

  // only need to perform incremental flushing when we have exceeded the total number of cached
  // bloom filters in memory
  if (cache_.size() > max_num_cached_buckets_)
  {
    std::vector<uint64_t> keys = GetOrderedKeys(cache_);

    // iterate through the cache pages that should be removed and flush them to disk if required
    std::size_t const num_keys_to_remove = cache_.size() - max_num_cached_buckets_;
    for (std::size_t i = 0; i < num_keys_to_remove; ++i)
    {
      // locate the page
      auto const it = cache_.find(keys[i]);
      assert(it != cache_.end());  // should not happen

      // flush the page if dirty
      if (it->second.dirty)
      {
        if (!SaveBucketToStore(it->first, it->second))
        {
          // can't flush the page to disk, for saftey keep the page in memory
          FETCH_LOG_ERROR(LOGGING_NAME, "Unable to flush bucket: ", it->first, " to disk");

          total_save_failures_->increment();
          continue;
        }

        ++pages_updated;
      }

      // remove the page from the cache
      cache_.erase(it);

      ++pages_flushed;
    }
  }

  // trigger a metadata update if we have flushed some pages
  if (pages_updated > 0)
  {
    UpdateMetadata();
    store_.Flush();
  }

  num_pages_in_memory_->set(cache_.size());

  return pages_flushed;
}

/**
 * Flush all the dirty
 */
void HistoricalBloomFilter::Flush()
{
  for (auto const &element : cache_)
  {
    if (element.second.dirty)
    {
      SaveBucketToStore(element.first, element.second);
    }
  }

  UpdateMetadata();
}

/**
 * Clear all data and start the file again
 */
void HistoricalBloomFilter::Reset()
{
  cache_.clear();
  store_.New(store_filename_);
  heaviest_persisted_bucket_ = 0;
}

/**
 * Convert a specified element index to a bucket index
 *
 * @param index The input element index
 * @return The calculated bucket index
 */
uint64_t HistoricalBloomFilter::ToBucket(uint64_t index) const
{
  return index / window_size_;
}

/**
 * Add an element to the specified buckets bloom filter entry
 *
 * @param element The element to be added
 * @param bucket The bucket to be added to
 */
void HistoricalBloomFilter::AddToBucket(ConstByteArray const &element, uint64_t bucket)
{
  auto &entry = cache_[bucket];

  // add the element to the filter and mark it as dirty
  if (!entry.filter)
  {
    entry.filter = std::make_unique<BasicBloomFilter>();
  }
  entry.filter->Add(element);

  // update the metadata
  entry.dirty = true;
}

/**
 * Match an element in a specified bloom filters bucket
 *
 * @param element The element to be matched against
 * @param bucket The specified bloom filter bucket to check
 * @return false if the element is not a match, otherwise true
 */
BloomFilterResult HistoricalBloomFilter::MatchInBucket(ConstByteArray const &element,
                                                       uint64_t              bucket) const
{
  // attempt to lookup the bucket in the cache
  auto const it = cache_.find(bucket);
  if (it != cache_.end())
  {
    // match the element against the one that is stored in memory
    return it->second.Match(element);
  }
  else
  {
    // only if we have a cache miss on the historical bloom filter do we need to check the
    // persistent bloom filter
    return MatchInStore(element, bucket);
  }
}

/**
 * Match an element to the specified bucket
 *
 * @param element The element to be matched
 * @param bucket The bucket to be checked
 * @return true if successful, otherwise false
 */
BloomFilterResult HistoricalBloomFilter::MatchInStore(ConstByteArray const &element,
                                                      uint64_t              bucket) const
{
  CacheEntry entry{};
  if (LookupBucketFromStore(bucket, entry))
  {
    return entry.Match(element);
  }

  return {false, 0};
}

/**
 * Read the information from the storage bucket from disk
 *
 * @param bucket The bucket to read
 * @param entry The output entry to populate
 * @return true if successful, otherwise false
 */
bool HistoricalBloomFilter::LookupBucketFromStore(uint64_t bucket, CacheEntry &entry) const
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Restoring cache entry to bucket: ", bucket);

  // load up the contents of the bloom filter
  ConstByteArray bloom_filter_buffer;
  if (!store_.Get(bucket, bloom_filter_buffer))
  {
    return false;
  }

  // deserialise the bloom filter entry
  bool success{false};
  try
  {
    serializers::MsgPackSerializer serialiser{bloom_filter_buffer};
    serialiser >> entry;

    success = true;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Error recovering bloom filter entry: ", ex.what());
  }

  return success;
}

/**
 * Save the contents of a bucket into the store
 *
 * @param bucket The bucket to be stored
 * @param entry The cache entry to be written
 * @return true if successful, otherwise false
 */
bool HistoricalBloomFilter::SaveBucketToStore(uint64_t bucket, CacheEntry const &entry)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Saving cache entry to bucket: ", bucket);

  // serialise the bloom filter buffer
  ConstByteArray bloom_filter_buffer{};
  try
  {
    serializers::LargeObjectSerializeHelper serialiser{};
    serialiser << entry;

    bloom_filter_buffer = serialiser.data();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Error serialising bloom filter entry: ", ex.what());
    return false;
  }

  // store the bloom filter into the bucket
  if (!store_.Set(bucket, bloom_filter_buffer))
  {
    return false;
  }

  // track the heaviest written bucket
  heaviest_persisted_bucket_ = std::max(heaviest_persisted_bucket_, bucket);

  return true;
}

/**
 * Updates the metadata on the file for tracking purposes
 */
void HistoricalBloomFilter::UpdateMetadata()
{
  BloomFilterMetadata metadata{};
  bool                bad_version = false;

  try
  {
    // ensure that the store is also
    store_.Flush();

    // before loading the database check that the meta data for the database is correct
    metadata_.Get(metadata);

    if (metadata.window_size != window_size_)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "The window size is not configured to match previous version!");
      bad_version = true;
    }

    if (metadata.last_bucket != heaviest_persisted_bucket_)
    {
      // update the metadata
      metadata.last_bucket = heaviest_persisted_bucket_;

      // flush to disk
      metadata_.Set(metadata);
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to update metadata: ", ex.what());
  }

  if (bad_version)
  {
    throw std::runtime_error("The window size is not configured to match previous version!");
  }
}

/**
 * Match a element in the bloom filter
 *
 * @param element The element to be matched
 * @return true if a valid value, otherwise false
 */
BloomFilterResult HistoricalBloomFilter::CacheEntry::Match(ConstByteArray const &element) const
{
  if (filter)
  {
    return filter->Match(element);
  }

  return {false, 0};
}

}  // namespace bloom

namespace serializers {

template <typename D>
struct MapSerializer<bloom::BloomFilterMetadata, D>
{
public:
  using Type       = bloom::BloomFilterMetadata;
  using DriverType = D;

  static uint8_t const WINDOW_SIZE = 1;
  static uint8_t const LAST_BUCKET = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &item)
  {
    auto map = map_constructor(2);
    map.Append(WINDOW_SIZE, item.window_size);
    map.Append(LAST_BUCKET, item.last_bucket);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &item)
  {
    map.ExpectKeyGetValue(WINDOW_SIZE, item.window_size);
    map.ExpectKeyGetValue(LAST_BUCKET, item.last_bucket);
  }
};

}  // namespace serializers

}  // namespace fetch
