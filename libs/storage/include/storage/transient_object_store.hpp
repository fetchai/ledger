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

#include "core/containers/queue.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "core/threading.hpp"
#include "storage/object_store.hpp"

#include <map>
#include <unordered_map>

namespace fetch {
namespace storage {

/**
 * The transient object store is a cached version of the object store, where objects that are likely
 * to be requested very soon after being written are stored in a cache. Once items are finished with
 * they can be 'confirmed', that is, written to the underlying object store.
 *
 * @tparam Object The type of the object being stored
 */
template <typename Object>
class TransientObjectStore
{
public:
  using Callback    = std::function<void(Object const &)>;
  using Archive     = ObjectStore<Object>;
  using TxSummaries = std::vector<ledger::TransactionSummary>;

  static constexpr char const *LOGGING_NAME = "TransientObjectStore";

  // Construction / Destruction
  TransientObjectStore();
  TransientObjectStore(TransientObjectStore const &) = delete;
  TransientObjectStore(TransientObjectStore &&)      = delete;
  ~TransientObjectStore();

  Archive &archive()
  {
    return archive_;
  }

  void New(std::string const &doc_file, std::string const &index_file, bool const &create = true);
  void Load(std::string const &doc_file, std::string const &index_file, bool const &create = true);

  /// @name Accessors
  /// @{
  bool        Get(ResourceID const &rid, Object &object);
  bool        Has(ResourceID const &rid);
  void        Set(ResourceID const &rid, Object const &object, bool newly_seen);
  bool        Confirm(ResourceID const &rid);
  TxSummaries GetRecent(uint32_t max_to_poll);
  /// @}

  void SetCallback(Callback cb)
  {
    set_callback_ = std::move(cb);
  }

  // Operators
  TransientObjectStore &operator=(TransientObjectStore const &) = delete;
  TransientObjectStore &operator=(TransientObjectStore &&) = delete;

private:
  using Mutex       = fetch::mutex::Mutex;
  using Queue       = fetch::core::MPMCQueue<ResourceID, 1 << 15>;
  using RecentQueue = fetch::core::MPMCQueue<ledger::TransactionSummary, 1 << 15>;
  using Cache       = std::unordered_map<ResourceID, Object>;
  using ThreadPtr   = std::shared_ptr<std::thread>;
  using Flag        = std::atomic<bool>;

  bool GetFromCache(ResourceID const &rid, Object &object);
  void SetInCache(ResourceID const &rid, Object const &object);
  bool IsInCache(ResourceID const &rid);
  void AddToWriteQueue(ResourceID const &rid);
  void ThreadLoop();

  mutable Mutex cache_mutex_{__LINE__, __FILE__};  ///< The mutex for the cache
  Cache         cache_;                            ///< The main object cache
  Archive       archive_;                          ///< The persistent object store
  Queue         confirm_queue_;                    ///< The queue of elements to be stored
  RecentQueue   most_recent_seen_;                 ///< The queue of elements to be stored
  ThreadPtr     thread_;                           ///< The background worker thread
  Callback      set_callback_;                     ///< The completion handler
  Flag          stop_{false};                      ///< Flag to signal the stop of the worker
  static constexpr core::Tickets::Count recent_queue_alarm_threshold{RecentQueue::QUEUE_LENGTH >>
                                                                     1};
};

template <typename O>
constexpr core::Tickets::Count TransientObjectStore<O>::recent_queue_alarm_threshold;

/**
 * Construct a transient object store
 *
 * @tparam O The type of the object being stored
 */
template <typename O>
inline TransientObjectStore<O>::TransientObjectStore()
{
  thread_ = std::make_shared<std::thread>([this] { ThreadLoop(); });
}

/**
 * Destructor
 *
 * @tparam O The type of the object being stored
 */
template <typename O>
inline TransientObjectStore<O>::~TransientObjectStore()
{
  if (thread_->joinable())
  {
    stop_ = true;
    thread_->join();
  }
}

/**
 * Initialise the storage engine (from scratch) using the specified paths
 *
 * @tparam O The type of the object being stored
 * @param doc_file The path to the document file
 * @param index_file The path to the index file
 * @param create Flag to indicate if the file should be created if it doesn't exist
 */
template <typename O>
void TransientObjectStore<O>::New(std::string const &doc_file, std::string const &index_file,
                                  bool const &create)
{
  archive_.New(doc_file, index_file, create);
}

/**
 * Initialise the storage engine from (potentially) existing data, using the specified paths
 *
 * @tparam O The type of the object being stored
 * @param doc_file
 * @param index_file
 * @param create
 */
template <typename O>
void TransientObjectStore<O>::Load(std::string const &doc_file, std::string const &index_file,
                                   bool const &create)
{
  archive_.Load(doc_file, index_file, create);
}

/**
 * Retrieve an object with the specified resource id
 *
 * @tparam O The type of the object being stored
 * @param rid The specified resource id to lookup
 * @param object The reference to the output object that is to be populated
 * @return true if the element was retrieved, otherwise false
 */
template <typename O>
bool TransientObjectStore<O>::Get(ResourceID const &rid, O &object)
{
  bool const success = GetFromCache(rid, object) || archive_.Get(rid, object);

  if (!success)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Unable to retrieve TX: ", byte_array::ToBase64(rid.id()));
  }

  return success;
}

/**
 * Get the recent transactions seen at the store (recently seen to the node/lane).
 *
 * @tparam O The type of the object being stored
 * @param max_to_poll the maximum number we want to return
 * @return a vector of the tx summaries
 */
template <typename O>
typename TransientObjectStore<O>::TxSummaries TransientObjectStore<O>::GetRecent(
    uint32_t max_to_poll)
{
  TransientObjectStore<O>::TxSummaries   ret;
  ledger::TransactionSummary             summary;
  static const std::chrono::milliseconds MAX_WAIT_INTERVAL{5};

  for (std::size_t i = 0; i < max_to_poll; ++i)
  {
    if (most_recent_seen_.Pop(summary, MAX_WAIT_INTERVAL))
    {
      ret.push_back(summary);
    }
    else
    {
      break;
    }
  }

  return ret;
}

/**
 * Check to see if the store has an element stored with the specified resource id
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id to be queried
 * @return true if the element exists, otherwise false
 */
template <typename O>
bool TransientObjectStore<O>::Has(ResourceID const &rid)
{
  return IsInCache(rid) || archive_.Has(rid);
}

/**
 * Set the value of an object with the specified resource id
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id (index) to be used for the element
 * @param object The value of the element to be stored
 */
template <typename O>
void TransientObjectStore<O>::Set(ResourceID const &rid, O const &object, bool newly_seen)
{
  static core::Tickets::Count prev_count{0};

  FETCH_LOG_DEBUG(LOGGING_NAME, "Adding TX: ", byte_array::ToBase64(rid.id()));

  // add the element into the cache
  SetInCache(rid, object);

  if (newly_seen)
  {
    std::size_t count{most_recent_seen_.QUEUE_LENGTH};
    bool const  inserted =
        most_recent_seen_.Push(object.summary(), count, std::chrono::milliseconds{100});
    if (inserted && prev_count != count)
    {
      if (prev_count < recent_queue_alarm_threshold && count >= recent_queue_alarm_threshold)
      {
        FETCH_LOG_WARN(LOGGING_NAME, " the `most_recent_seen_` queue size ", count,
                       " reached or is over threshold ", recent_queue_alarm_threshold, ").");
        // TODO(private issue #582): The queue became FULL - this information shall
        // be propagated out to caller, so it can make appropriate decision how to
        // proceed.
      }
      prev_count = count;
    }
  }

  // dispatch the callback if necessary
  if (set_callback_)
  {
    set_callback_(object);
  }
}

/**
 * Once we are sure the object should be written to disk we confirm it. This takes the form
 * of notifying a worker thread which writes to disk as fast as possible.
 *
 * @param: rid The resource id for the object
 *
 * @return: bool Whether the object was confirmed (or scheduled) from the cache into the underlying
 * store. Note, there can be races if this function is called multiple times with the same RID, this
 *               is not the intended usage.
 */
template <typename O>
bool TransientObjectStore<O>::Confirm(ResourceID const &rid)
{
  if (!IsInCache(rid))
  {
    return false;
  }

  // add the element into the queue of items to be pushed to disk
  AddToWriteQueue(rid);

  return true;
}

/**
 * Internal: Lookup an element from the cache
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id to be queried
 * @param object The reference to the output object that is to be populated
 * @return true if the object was found, otherwise false
 */
template <typename O>
bool TransientObjectStore<O>::GetFromCache(ResourceID const &rid, O &object)
{
  FETCH_LOCK(cache_mutex_);
  bool success = false;

  auto it = cache_.find(rid);
  if (it != cache_.end())
  {
    object  = it->second;
    success = true;
  }

  return success;
}

/**
 * Internal: Set an element into the cache
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id of the element to be set
 * @param object The reference to the object to be stored
 */
template <typename O>
void TransientObjectStore<O>::SetInCache(ResourceID const &rid, O const &object)
{
  FETCH_LOCK(cache_mutex_);
  typename Cache ::iterator it;
  bool                      inserted{false};

  // attempt to insert the element into the map
  std::tie(it, inserted) = cache_.emplace(rid, object);

  // check to see if the insertion was successful
  if (!inserted)
  {
    // update the entry
    it->second = object;
  }
}

/**
 * Internal: Check to see if an element is in the cache
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id of the element
 * @return
 */
template <typename O>
bool TransientObjectStore<O>::IsInCache(ResourceID const &rid)
{
  FETCH_LOCK(cache_mutex_);
  return cache_.find(rid) != cache_.end();
}

/**
 * Internal: Signal that the item needs to be stored permanently
 *
 * @tparam O The type of the object being stored
 * @param rid the resource identifier to be put in the queue
 */
template <typename O>
void TransientObjectStore<O>::AddToWriteQueue(ResourceID const &rid)
{
  confirm_queue_.Push(rid);
}

/**
 * Thread loop is executed by a thread and writes confirmed objects to the object store
 */
template <typename O>
void TransientObjectStore<O>::ThreadLoop()
{
  static const std::size_t               BATCH_SIZE = 100;
  static const std::chrono::milliseconds MAX_WAIT_INTERVAL{2000};

  SetThreadName("TxStore");

  std::vector<ResourceID> rids(BATCH_SIZE);
  std::size_t             extracted_count = 0;
  std::size_t             written_count   = 0;

  enum class Phase
  {
    Populating,
    Writing,
    Flushing
  };

  Phase phase = Phase::Populating;
  O     obj;

  // main processing loop
  while (!stop_)
  {
    switch (phase)
    {
    // Populating: We are filling up our batch of objects from the queue that is being posted
    case Phase::Populating:
    {
      assert(extracted_count < BATCH_SIZE);

      // ensure the write count is reset
      written_count = 0;

      // attempt to extract an element in the confirmation queue
      bool const extracted = confirm_queue_.Pop(rids[extracted_count], MAX_WAIT_INTERVAL);

      // update the index if needed
      if (extracted)
      {
        ++extracted_count;
      }

      // determine if we have reached the end of the sequence of transactions or if we have a filled
      // buffer.
      bool const is_buffer_full     = (extracted_count == BATCH_SIZE);
      bool const is_end_of_sequence = (extracted_count && (!extracted));

      // trigger the next state if appropriate
      if (is_buffer_full || is_end_of_sequence)
      {
        phase = Phase::Writing;
      }

      break;
    }

    // Writing: We are extracting the items from the cache and writing them to disk
    case Phase::Writing:
    {
      // check if we need to transition from this state
      if (written_count >= extracted_count)
      {
        phase = Phase::Flushing;
      }
      else
      {
        auto const &rid = rids[written_count];

        // get the element from the cache
        if (GetFromCache(rid, obj))
        {
          // write out the object
          archive_.Set(rid, obj);

          ++written_count;
        }
        else
        {
          // If this is the case then for some reason the RID that was added to the queue has been
          // removed from the cache.
          assert(false);
        }
      }

      break;
    }

    // Flushing: In this phase we are removing the elements from the cache. This is important to
    // ensure a bound on the memory resources. This must happen after the writing to disk,
    // otherwise the object store will be inconsistent
    case Phase::Flushing:
    {
      FETCH_LOCK(cache_mutex_);
      assert(extracted_count <= BATCH_SIZE);
      for (std::size_t i = 0; i < extracted_count; ++i)
      {
        cache_.erase(rids[i]);
      }

      phase           = Phase::Populating;
      extracted_count = 0;

      break;
    }
    }
  }
}

}  // namespace storage
}  // namespace fetch
