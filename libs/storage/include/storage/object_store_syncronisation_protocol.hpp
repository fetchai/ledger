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

#include "core/logger.hpp"
#include "network/details/thread_pool.hpp"
#include "network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include "storage/object_store.hpp"
#include "storage/resource_mapper.hpp"

#include <set>
#include <utility>
#include <vector>
namespace fetch {
namespace storage {

template <typename R, typename T, typename S = T>
class ObjectStoreSyncronisationProtocol : public fetch::service::Protocol
{
public:
  enum
  {
    OBJECT_COUNT = 1,
    PULL_OBJECTS = 2,
    PULL_SUBTREE = 3
  };
  using self_type             = ObjectStoreSyncronisationProtocol<R, T, S>;
  using protocol_handler_type = service::protocol_handler_type;
  using register_type         = R;
  using thread_pool_type      = network::ThreadPool;

  ObjectStoreSyncronisationProtocol(protocol_handler_type const &p, register_type r,
                                    thread_pool_type nm, ObjectStore<T> *store)
    : fetch::service::Protocol()
    , protocol_(p)
    , register_(std::move(r))
    , thread_pool_(std::move(nm))
    , store_(store)
  {

    this->Expose(OBJECT_COUNT, this, &self_type::ObjectCount);
    this->ExposeWithClientArg(PULL_OBJECTS, this, &self_type::PullObjects);
    this->Expose(PULL_SUBTREE, this, &self_type::PullSubtree);
  }

  void Start()
  {
    fetch::logger.Debug("Starting synchronisation of ", typeid(T).name());
    if (running_) return;
    running_ = true;
    thread_pool_->Post([this]() { this->IdleUntilPeers(); });
  }

  void Stop() { running_ = false; }

  /**
   * Spin until the connected peers is adequate
   */
  void IdleUntilPeers()
  {
    if (!running_) return;

    if (register_.number_of_services() == 0)
    {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); },
                         1000);  // TODO(issue 9): Make time variable
    }
    else
    {
#if 1
      // If we need to sync our object store (esp. when joining the network)
      if (needs_sync_)
      {
        for (uint16_t i = 0; i < 0x100; ++i)
        {
          roots_to_sync_.push(static_cast<uint8_t>(i));
        }

        thread_pool_->Post([this]() { this->SyncSubtree(); });
      }
      else
      {
        thread_pool_->Post([this]() { this->FetchObjectsFromPeers(); });
      }
#endif
    }
  }

  void FetchObjectsFromPeers()
  {
    fetch::logger.Debug("Fetching objects ", typeid(T).name(), " from peer");

    if (!running_) return;

    std::lock_guard<mutex::Mutex> lock(object_list_mutex_);

    using service_map_type = typename R::service_map_type;
    register_.WithServices([this](service_map_type const &map) {
      for (auto const &p : map)
      {
        if (!running_) return;

        auto peer = p.second;
        auto ptr  = peer.lock();
        object_list_promises_.push_back(ptr->Call(protocol_, PULL_OBJECTS));
      }
    });

    if (running_)
    {
      thread_pool_->Post([this]() { this->RealisePromises(); });
    }
  }

  void RealisePromises()
  {
    if (!running_) return;
    std::lock_guard<mutex::Mutex> lock(object_list_mutex_);
    incoming_objects_.reserve(uint64_t(max_cache_));

    new_objects_.clear();

    for (auto &p : object_list_promises_)
    {

      if (!running_) return;

      incoming_objects_.clear();
      FETCH_LOG_PROMISE();
      if (!p.Wait(1000, false))
      {
        continue;
      }

      p.template As<std::vector<S>>(incoming_objects_);

      if (!running_) return;
      std::lock_guard<mutex::Mutex> lock(mutex_);

      store_->WithLock([this]() {
        for (auto &o : incoming_objects_)
        {
          CachedObject obj;
          obj.data = T::Create(o);
          ResourceID rid(obj.data.digest());

          if (store_->LocklessHas(rid)) continue;
          store_->LocklessSet(rid, obj.data);

          cache_.push_back(obj);
        }
      });
    }

    object_list_promises_.clear();
    if (running_)
    {
      thread_pool_->Post([this]() { this->TrimCache(); });
    }
  }

  void TrimCache()
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    std::size_t                   i = 0;
    for (auto &c : cache_)
    {
      c.UpdateLifetime();
    }
    std::sort(cache_.begin(), cache_.end());

    while ((i < cache_.size()) &&
           ((cache_.size() > max_cache_) || (cache_.back().lifetime > max_cache_life_time_)))
    {
      auto back = cache_.back();
      //      std::cout << "DELETEING Object with " << back.lifetime << " ms
      //      lifetime" << std::endl;
      cache_.pop_back();
    }

    if (running_)
    {
      // TODO(issue 9): Make time parameter
      thread_pool_->Post([this]() { this->IdleUntilPeers(); }, 5000);
    }
  }

  /// @}

  void AddToCache(T const &o)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    CachedObject                  obj;
    obj.data = o;
    cache_.push_back(obj);
  }

  /**
   * Allow peers to pull large sections of your subtree for synchronisation on entry to the network
   *
   * @param: client_handle Handle referencing client making the request
   *
   * @return: the subtree the client is requesting as a vector (size limited)
   */
  std::vector<S> PullSubtree(byte_array::ConstByteArray const &rid, uint64_t mask)
  {
    std::vector<S> ret;

    uint64_t counter = 0;

    store_->WithLock([this, &ret, &counter, &rid, mask]() {
      // This is effectively saying get all objects whose ID begins rid & mask
      auto it = store_->GetSubtree(ResourceID(rid), mask);

      while (it != store_->end() && counter++ < PULL_LIMIT_)
      {
        ret.push_back(*it);
        ++it;
      }
    });

    return ret;
  }

private:
  protocol_handler_type protocol_;
  register_type         register_;
  thread_pool_type      thread_pool_;
  const uint64_t        PULL_LIMIT_ = 100000;  // Limit the amount a single rpc call will provide

  uint64_t ObjectCount()
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return cache_.size();  // TODO(issue 9): Return store size
  }

  std::vector<S> PullObjects(uint64_t const &client_handle)
  {

    std::lock_guard<mutex::Mutex> lock(mutex_);

    /*
    for(auto &c: cache_) {
      std::cout << "--- " << byte_array::ToBase64( c.data.digest()) <<
    std::endl;
    }
    */

    if (cache_.begin() == cache_.end())
    {
      return std::vector<S>();
    }

    // Creating result
    std::vector<S> ret;

    for (auto &c : cache_)
    {
      if (c.delivered_to.find(client_handle) == c.delivered_to.end())
      {
        c.delivered_to.insert(client_handle);
        ret.push_back(c.data);
      }
    }
    //    std::cout << "Sending " << ret.size() << std::endl;

    return ret;
  }

  struct CachedObject
  {
    CachedObject() { created = std::chrono::system_clock::now(); }

    void UpdateLifetime()
    {
      std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
      lifetime =
          double(std::chrono::duration_cast<std::chrono::milliseconds>(end - created).count());
    }

    bool operator<(CachedObject const &other) const { return lifetime < other.lifetime; }

    T                                     data;
    std::unordered_set<uint64_t>          delivered_to;
    std::chrono::system_clock::time_point created;
    double                                lifetime = 0;
  };

  // Create a stack of subtrees we want to sync. Push roots back onto this when the promise
  // fails. Completion when the stack is empty
  void SyncSubtree()
  {
    if (!running_) return;

    using service_map_type = typename R::service_map_type;

    register_.WithServices([this](service_map_type const &map) {
      for (auto const &p : map)
      {
        if (!running_) return;

        auto peer = p.second;
        auto ptr  = peer.lock();

        if (roots_to_sync_.empty())
        {
          break;
        }

        auto root = roots_to_sync_.front();
        roots_to_sync_.pop();

        byte_array::ByteArray array;
        array.Resize(256 / 8);
        array[0] = root;

        auto promise = ptr->Call(protocol_, PULL_SUBTREE, array, 8);
        subtree_promises_.push_back(std::make_pair(root, std::move(promise)));
      }
    });

    thread_pool_->Post([this]() { this->RealiseSubtreePromises(); }, 500);
  }

  void RealiseSubtreePromises()
  {
    for (auto &promise_pair : subtree_promises_)
    {
      auto &root    = promise_pair.first;
      auto &promise = promise_pair.second;

      // Timeout fail, push this subtree back onto the root for another go
      incoming_objects_.clear();
      if (!promise.Wait(100, false))
      {
        roots_to_sync_.push(root);
        continue;
      }

      promise.template As<std::vector<S>>(incoming_objects_);

      store_->WithLock([this]() {
        for (auto &o : incoming_objects_)
        {
          CachedObject obj;
          obj.data = T::Create(o);
          ResourceID rid(obj.data.digest());

          if (store_->LocklessHas(rid)) continue;
          store_->LocklessSet(rid, obj.data);

          cache_.push_back(obj);
        }
      });
    }

    subtree_promises_.clear();

    // Completed syncing
    if (roots_to_sync_.empty())
    {
      thread_pool_->Post([this]() { this->IdleUntilPeers(); });
    }
    else
    {
      thread_pool_->Post([this]() { this->SyncSubtree(); });
    }
  }

  mutex::Mutex    mutex_{__LINE__, __FILE__};
  ObjectStore<T> *store_;

  std::vector<CachedObject> cache_;

  uint64_t max_cache_           = 2000;
  double   max_cache_life_time_ = 20000;  // TODO(issue 7): Make cache configurable

  mutable mutex::Mutex          object_list_mutex_{__LINE__, __FILE__};
  std::vector<service::Promise> object_list_promises_;
  std::vector<T>                new_objects_;
  std::vector<S>                incoming_objects_;

  std::atomic<bool> running_{false};

  // Syncing with other peers on startup
  bool                                              needs_sync_ = true;
  std::vector<std::pair<uint8_t, service::Promise>> subtree_promises_;
  std::queue<uint8_t>                               roots_to_sync_;
};

}  // namespace storage
}  // namespace fetch
