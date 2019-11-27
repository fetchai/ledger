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

#include "chain/transaction_layout.hpp"
#include "core/containers/queue.hpp"
#include "core/mutex.hpp"
#include "core/runnable.hpp"
#include "core/set_thread_name.hpp"
#include "core/state_machine.hpp"
#include "logging/logging.hpp"
#include "storage/object_store.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/registry.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
class TransientObjectStore2
{
public:
  using Callback     = std::function<void(Object const &)>;
  using Archive      = ObjectStore<Object>;
  using TxLayouts    = std::vector<chain::TransactionLayout>;
  using TxArray      = std::vector<chain::Transaction>;
  using WeakRunnable = core::WeakRunnable;

  static constexpr char const *LOGGING_NAME = "TransientObjectStore";

  explicit TransientObjectStore2(uint32_t log2_num_lanes);
  TransientObjectStore2(TransientObjectStore2 const &) = delete;
  TransientObjectStore2(TransientObjectStore2 &&)      = delete;

  void New(std::string const &doc_file, std::string const &index_file, bool const &create = true);
  void Load(std::string const &doc_file, std::string const &index_file, bool const &create = true);

  /// @name Accessors
  /// @{
  bool      Get(ResourceID const &rid, Object &object);
  bool      Has(ResourceID const &rid);
  void      Set(ResourceID const &rid, Object const &object, bool newly_seen);
  bool      Confirm(ResourceID const &rid);
  TxLayouts GetRecent(uint32_t max_to_poll);
  /// @}

  void SetCallback(Callback cb)
  {
    set_callback_ = std::move(cb);
  }

  // Operators
  TransientObjectStore2 &operator=(TransientObjectStore2 const &) = delete;
  TransientObjectStore2 &operator=(TransientObjectStore2 &&) = delete;

  std::size_t Size() const;

  TxArray PullSubtree(byte_array::ConstByteArray const &rid, uint64_t bit_count,
                      uint64_t pull_limit);

  WeakRunnable GetWeakRunnable() const;

private:
  enum class Phase
  {
    Populating,
    Writing,
  };

  using StateMachinePtr = std::shared_ptr<core::StateMachine<Phase>>;
  using Queue           = fetch::core::MPMCQueue<ResourceID, 1 << 15>;
  using RecentQueue     = fetch::core::MPMCQueue<chain::TransactionLayout, 1 << 15>;
  using Cache           = std::unordered_map<ResourceID, Object>;
  using Flag            = std::atomic<bool>;

  bool GetFromCache(ResourceID const &rid, Object &object);
  void SetInCache(ResourceID const &rid, Object const &object);
  bool IsInCache(ResourceID const &rid);

  Phase OnPopulating();
  Phase OnWriting();
  Phase OnFlushing();

  uint32_t const    log2_num_lanes_;
  std::size_t const batch_size_ = 100;

  /// @name State Machine States
  /// @{
  std::vector<ResourceID> rids_;
  /// @}

  mutable Mutex   cache_mutex_;       ///< The mutex for the cache
  StateMachinePtr state_machine_;     ///< The state machine controlling the worker writing to disk
  Cache           cache_;             ///< The main object cache
  Archive         archive_;           ///< The persistent object store
  Queue           confirm_queue_;     ///< The queue of elements to be stored
  RecentQueue     most_recent_seen_;  ///< The queue of elements to be stored
  Callback        set_callback_;      ///< The completion handler
  Flag            stop_{false};       ///< Flag to signal the stop of the worker
  core::Tickets::Count                  recent_queue_last_size_{0};
  static constexpr core::Tickets::Count recent_queue_alarm_threshold{RecentQueue::QUEUE_LENGTH >>
                                                                     1};

  /// @name Telemetry
  /// @{
  telemetry::CounterPtr cache_rid_removed_;
  /// @}
};

/**
 * Construct a transient object store
 *
 * @tparam O The type of the object being stored
 */
template <typename O>
TransientObjectStore2<O>::TransientObjectStore2(uint32_t log2_num_lanes)
  : log2_num_lanes_(log2_num_lanes)
  , state_machine_{std::make_shared<core::StateMachine<Phase>>("TransientObjectStore",
                                                               Phase::Populating)}
  , cache_rid_removed_{telemetry::Registry::Instance().CreateCounter(
        "ledger_storage_transient_rid_removed_total",
        "The number of needed rids which were removed from cache.")}

{
  rids_.reserve(batch_size_);

  state_machine_->RegisterHandler(Phase::Populating, this, &TransientObjectStore2<O>::OnPopulating);
  state_machine_->RegisterHandler(Phase::Writing, this, &TransientObjectStore2<O>::OnWriting);
}

template <typename O>
std::size_t TransientObjectStore2<O>::Size() const
{
  FETCH_LOCK(cache_mutex_);

  return archive_.size() + cache_.size();
}

template <typename O>
typename TransientObjectStore2<O>::TxArray TransientObjectStore2<O>::PullSubtree(
    byte_array::ConstByteArray const &rid, uint64_t bit_count, uint64_t pull_limit)
{
  TxArray ret{};

  uint64_t counter = 0;

  archive_.Flush(false);

  archive_.WithLock([this, &pull_limit, &ret, &counter, &rid, bit_count]() {
    // This is effectively saying get all objects whose ID begins rid & mask
    auto it = this->archive_.GetSubtree(ResourceID(rid), bit_count);

    while ((it != this->archive_.end()) && (counter++ < pull_limit))
    {
      ret.push_back(*it);
      ++it;
    }
  });

  return ret;
}

template <typename O>
constexpr core::Tickets::Count TransientObjectStore2<O>::recent_queue_alarm_threshold;

// Populating: We are filling up our batch of objects from the queue that is being posted
template <typename O>
typename TransientObjectStore2<O>::Phase TransientObjectStore2<O>::OnPopulating()
{
  for (;;)
  {
    // attempt to extract an element in the confirmation queue
    rids_.emplace_back(ResourceID{});
    bool const extracted = confirm_queue_.Pop(rids_.back(), std::chrono::milliseconds::zero());

    // update the index if needed
    if (!extracted)
    {
      rids_.pop_back();
    }

    bool const is_buffer_full    = (rids_.size() == batch_size_);
    bool const is_batch_complete = (!extracted) && (!rids_.empty());

    if (is_buffer_full || is_batch_complete)
    {
      return Phase::Writing;
    }

    // Queue is empty and nothing to write - trigger delay and do not change FSM state
    if (!extracted)
    {
      state_machine_->Delay(std::chrono::milliseconds(1000u));
      break;
    }
  }

  return Phase::Populating;
}

// Writing: We are extracting the items from the cache and writing them to disk
template <typename O>
typename TransientObjectStore2<O>::Phase TransientObjectStore2<O>::OnWriting()
{
  // check if we need to transition from this state
  if (rids_.empty())
  {
    return Phase::Populating;
  }

  O           obj;
  auto const &rid = rids_.back();

  FETCH_LOCK(cache_mutex_);

  // get the element from the cache
  if (GetFromCache(rid, obj))
  {
    // write out the object
    archive_.Set(rid, obj);

    // remove it the cache also
    cache_.erase(rid);
  }
  else
  {
    // If this is the case then for some reason the RID that was added
    // to the queue has been removed from the cache.
    FETCH_LOG_WARN(LOGGING_NAME,
                   "RID that was added to the queue has been removed from the cache.");
    cache_rid_removed_->increment();
    assert(false);
  }

  // remove the current element
  rids_.pop_back();

  return Phase::Writing;
}

template <typename O>
core::WeakRunnable TransientObjectStore2<O>::GetWeakRunnable() const
{
  return state_machine_;
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
void TransientObjectStore2<O>::New(std::string const &doc_file, std::string const &index_file,
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
void TransientObjectStore2<O>::Load(std::string const &doc_file, std::string const &index_file,
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
bool TransientObjectStore2<O>::Get(ResourceID const &rid, O &object)
{
  bool success = false;

  {
    FETCH_LOCK(cache_mutex_);
    success = GetFromCache(rid, object) || archive_.Get(rid, object);
  }

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
typename TransientObjectStore2<O>::TxLayouts TransientObjectStore2<O>::GetRecent(
    uint32_t max_to_poll)
{
  static const std::chrono::milliseconds MAX_WAIT{5};

  TxLayouts                layouts{};
  chain::TransactionLayout summary;

  for (std::size_t i = 0; i < max_to_poll; ++i)
  {
    if (most_recent_seen_.Pop(summary, MAX_WAIT))
    {
      layouts.push_back(summary);
    }
    else
    {
      break;
    }
  }

  return layouts;
}

/**
 * Check to see if the store has an element stored with the specified resource id
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id to be queried
 * @return true if the element exists, otherwise false
 */
template <typename O>
bool TransientObjectStore2<O>::Has(ResourceID const &rid)
{
  FETCH_LOCK(cache_mutex_);

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
void TransientObjectStore2<O>::Set(ResourceID const &rid, O const &object, bool newly_seen)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Adding TX: ", byte_array::ToBase64(rid.id()));

  {
    FETCH_LOCK(cache_mutex_);

    SetInCache(rid, object);
  }

  if (newly_seen)
  {
    std::size_t count{decltype(most_recent_seen_)::QUEUE_LENGTH};
    bool const  inserted = most_recent_seen_.Push(chain::TransactionLayout{object, log2_num_lanes_},
                                                 count, std::chrono::milliseconds{100});
    if (inserted && recent_queue_last_size_ != count)
    {
      // TODO(private issue #582): The queue became FULL - this information shall
      // be propagated out to caller, so it can make appropriate decision how to
      // proceed.
      if (recent_queue_last_size_ < recent_queue_alarm_threshold &&
          count >= recent_queue_alarm_threshold)
      {
        FETCH_LOG_WARN(LOGGING_NAME, " the `most_recent_seen_` queue size ", count,
                       " reached or is over threshold ", recent_queue_alarm_threshold, ").");
      }
      else if (count < recent_queue_alarm_threshold &&
               recent_queue_last_size_ >= recent_queue_alarm_threshold)
      {
        FETCH_LOG_WARN(LOGGING_NAME, " the `most_recent_seen_` queue size ", count,
                       " dropped bellow threshold ", recent_queue_alarm_threshold, ").");
      }
      recent_queue_last_size_ = count;
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
bool TransientObjectStore2<O>::Confirm(ResourceID const &rid)
{
  {
    FETCH_LOCK(cache_mutex_);

    if (!IsInCache(rid))
    {
      return false;
    }
  }

  // add the element into the queue of items to be pushed to disk
  confirm_queue_.Push(rid);

  return true;
}

/**
 * Internal: Look up an element from the cache
 *
 * Note: Not thread safe. Always lock cache_mutex_ before
 * calling this function.
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id to be queried
 * @param object The reference to the output object that is to be populated
 * @return true if the object was found, otherwise false
 */
template <typename O>
bool TransientObjectStore2<O>::GetFromCache(ResourceID const &rid, O &object)
{
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
 * Note: Not thread safe. Always lock cache_mutex_ before
 * calling this function.
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id of the element to be set
 * @param object The reference to the object to be stored
 */
template <typename O>
void TransientObjectStore2<O>::SetInCache(ResourceID const &rid, O const &object)
{
  typename Cache::iterator it;
  bool                     inserted{false};

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
 * Note: Not thread safe. Always lock cache_mutex_ before
 * calling this function.
 *
 * @tparam O The type of the object being stored
 * @param rid The resource id of the element
 * @return
 */
template <typename O>
bool TransientObjectStore2<O>::IsInCache(ResourceID const &rid)
{
  return cache_.find(rid) != cache_.end();
}

}  // namespace storage
}  // namespace fetch
