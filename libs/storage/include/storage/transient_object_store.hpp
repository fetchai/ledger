#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "storage/object_store.hpp"
#include "core/mutex.hpp"
#include "core/containers/queue.hpp"

#include <unordered_map>

namespace fetch {
namespace storage {

/**
 * The transient object store is a cached version of the object store, where objects that are likely to be requested
 * very soon after being written are stored in a cache. Once items are finished with they can be 'confirmed',
 * that is, written to the underlying object store.
 *
 * @tparam O The object being stored in the object store
 */
template <typename Object>
class TransientObjectStore
{
  using ThreadPtr  = std::shared_ptr<std::thread>;
  ThreadPtr thread_;

public:
  using Archive = ObjectStore<Object>;

  // Construction / Destruction
  TransientObjectStore()
  {
    thread_ = std::make_shared<std::thread>([this]{ThreadLoop();});
  }

  TransientObjectStore(TransientObjectStore const &) = delete;
  TransientObjectStore(TransientObjectStore &&) = delete;

  ~TransientObjectStore()
  {
    if(thread_->joinable())
    {
      // TODO: (`HUT`) : tidy the implementation of this up
      stop_ = true;
      confirm_queue_.Push(ResourceAddress{"0"});
      thread_->join();
    }
  }

  Archive &archive() { return archive_; }

  void New(std::string const &doc_file, std::string const &index_file, bool const &create = true);
  void Load(std::string const &doc_file, std::string const &index_file, bool const &create = true);

  /// @name Accessors
  /// @{
  bool Get(ResourceID const &rid, Object &object);
  bool Has(ResourceID const &rid);
  void Set(ResourceID const &rid, Object const &object);
  bool Confirm(ResourceID const &rid);
  /// @}

  // Operators
  TransientObjectStore &operator=(TransientObjectStore const &) = delete;
  TransientObjectStore &operator=(TransientObjectStore &&) = delete;

private:

  using Mutex = fetch::mutex::Mutex;
  using Queue = fetch::core::MPSCQueue<ResourceID, 1 << 13>;

  void ThreadLoop();

  struct CacheEntry
  {
    Object      obj;
    std::size_t writes{0};
    std::size_t reads{0};

    bool expired() const { return reads >= writes; }

    CacheEntry(Object const &o) : obj{o} {}
  };

  using Cache   = std::unordered_map<ResourceID, CacheEntry>;

  bool GetFromCache(ResourceID const &rid, Object &object);
  void SetInCache(ResourceID const &rid, Object const &object);
  bool IsInCache(ResourceID const &rid);
  void AddToWriteQueue(ResourceID const &rid);

  Cache   cache_;
  Archive archive_;

  mutable Mutex cache_mutex_{__LINE__, __FILE__};
  Queue confirm_queue_;
  bool stop_ = false;
};

template <typename O>
void TransientObjectStore<O>::New(std::string const &doc_file, std::string const &index_file, bool const &create)
{
  archive_.New(doc_file, index_file, create);
}

template <typename O>
void TransientObjectStore<O>::Load(std::string const &doc_file, std::string const &index_file, bool const &create)
{
  archive_.Load(doc_file, index_file, create);
}

template <typename O>
bool TransientObjectStore<O>::Get(ResourceID const &rid, O &object)
{
  return GetFromCache(rid, object) || archive_.Get(rid, object);
}

template <typename O>
bool TransientObjectStore<O>::Has(ResourceID const &rid)
{
  return IsInCache(rid) || archive_.Has(rid);
}

template <typename O>
void TransientObjectStore<O>::Set(ResourceID const &rid, O const &object)
{
  // add the element into the cache
  SetInCache(rid, object);
}

template <typename O>
bool TransientObjectStore<O>::GetFromCache(ResourceID const &rid, O &object)
{
  FETCH_LOCK(cache_mutex_);
  bool success = false;

  auto it = cache_.find(rid);
  if (it != cache_.end())
  {
    object = it->second.obj;
    it->second.reads++;
    success = true;
  }

  return success;
}

template <typename O>
void TransientObjectStore<O>::SetInCache(ResourceID const &rid, O const &object)
{
  FETCH_LOCK(cache_mutex_);
  typename Cache ::iterator it;
  bool inserted{false};

  // attempt to insert the element into the map
  std::tie(it, inserted) = cache_.emplace(rid, object);

  // check to see if the insertion was successful
  if (!inserted)
  {
    // update the entry
    it->second = object;
  }
}

template <typename O>
bool TransientObjectStore<O>::IsInCache(ResourceID const &rid)
{
  FETCH_LOCK(cache_mutex_);
  return cache_.find(rid) != cache_.end();
}

/**
 * Once we are sure the object should be written to disk we confirm it. This takes the form
 * of notifying a worker thread which writes to disk as fast as possible.
 *
 * @param: rid The resource id for the object
 *
 * @return: bool Whether the object was confirmed (or scheduled) from the cache into the underlying store.
 *               Note, there can be races if this function is called multiple times with the same RID, this
 *               is not the intended usage.
 */
template <typename O>
bool TransientObjectStore<O>::Confirm(ResourceID const &rid)
{
  if(!IsInCache(rid))
  {
    return false;
  }

  // add the element into the queue of items to be pushed to disk
  AddToWriteQueue(rid);

  return true;
}

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
  constexpr std::size_t bulk_size_max = 100;
  std::vector<ResourceID> items_to_confirm;

  while(!stop_)
  {
    items_to_confirm.clear();

    // Take up to bulk RIDs from the queue
    while(items_to_confirm.size() < bulk_size_max && !stop_)
    {
      auto rid = confirm_queue_.Pop();
      items_to_confirm.push_back(rid);
    }

    O placeholder_obj;

    // Push to underlying obj. store
    for(auto const &rid : items_to_confirm)
    {
      if(GetFromCache(rid, placeholder_obj))
      {
        archive_.Set(rid, placeholder_obj);
      }
    }
  }
}

} // namespace storage
} // namespace fetch
