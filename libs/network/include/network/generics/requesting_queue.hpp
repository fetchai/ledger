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

#include "network/service/promise.hpp"
#include "network/generics/promise_of.hpp"
#include "core/serializers/stl_types.hpp"

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <deque>

namespace fetch {
namespace network {

template<typename K, typename P, typename C = std::vector<P>>
class RequestingQueueOf
{
public:
  using Key                  = K;
  using Promised             = P;
  using ContainerOfPromised  = C;
  using Mutex                = fetch::mutex::Mutex;
  using Lock                 = std::lock_guard<Mutex>;
  using PromiseCollection    = PromiseOf<ContainerOfPromised>;
  using Promise              = PromiseOf<Promised>;
  using PromiseState         = fetch::service::PromiseState;
  using PromiseCollectionMap = std::unordered_map<Key, PromiseCollection>;
  using PromiseMap           = std::unordered_map<Key, Promise>;
  using KeySet               = std::unordered_set<Key>;
  using Counter              = std::atomic<std::size_t>;

  struct Result
  {
    Key       key;
    Promised  promised;
  };

  using Results = std::deque<Result>;

  // Construction / Destruction
  RequestingQueueOf() = default;
  RequestingQueueOf(RequestingQueueOf const &) = delete;
  RequestingQueueOf(RequestingQueueOf &&) = delete;
  ~RequestingQueueOf() = default;

  bool Add(Key const &key, PromiseCollection const &promises);
  bool Add(Key const &key, Promise const &promise);

  Results Get(std::size_t limit);

  KeySet FilterOutInFlight(KeySet const &inputs);
  bool IsInFlight(Key const &key) const;

  void Resolve();

#if 0
  std::size_t size() const;
#endif
  bool HasCompletedPromises() const;

  // Operators
  RequestingQueueOf &operator=(RequestingQueueOf const &) = delete;
  RequestingQueueOf &operator=(RequestingQueueOf &&) = delete;

private:

  mutable Mutex        mutex_{__LINE__, __FILE__};
  PromiseCollectionMap collection_requests_;
  PromiseMap           single_requests_;                  ///< The map of currently monitored promises
  Results              completed_;                 ///< The map of completed promises
  KeySet               failed_;                    ///< The set of failed keys
};

template<typename K, typename P, typename C>
bool RequestingQueueOf<K,P,C>::Add(Key const &key, PromiseCollection const &promises)
{
  bool success = false;

  {
    FETCH_LOCK(mutex_);
    if (collection_requests_.find(key) == collection_requests_.end())
    {
      collection_requests_.emplace(key, promises);
      success = true;
    }
  }

  return success;
}

template<typename K, typename P, typename C>
bool RequestingQueueOf<K,P,C>::Add(Key const &key, Promise const &promise)
{
  bool success = false;

  {
    FETCH_LOCK(mutex_);

    if (single_requests_.find(key) == single_requests_.end())
    {
      single_requests_.emplace(key, promise);
      success = true;
    }
  }

  return success;
}

template<typename K, typename P, typename C>
typename RequestingQueueOf<K,P,C>::Results RequestingQueueOf<K,P,C>::Get(std::size_t limit)
{
  FETCH_LOCK(mutex_);

  Results results;

  if (limit == 0)
  {
    assert(false);
  }
  else if (completed_.size() <= limit)
  {
    results = std::move(completed_);
    completed_.clear(); // needed?
  }
  else // (completed_.size() > limit)
  {
    // copy "limit" number of entries from the completed list
    std::copy_n(completed_.begin(), limit, std::inserter(results, results.begin()));

    // erase these entries from the completed map
    completed_.erase(completed_.begin(), completed_.begin() + static_cast<std::ptrdiff_t>(limit));
  }

  return results;
}

template<typename K, typename P, typename C>
void RequestingQueueOf<K,P,C>::Resolve()
{
  FETCH_LOCK(mutex_);

  {
    auto iter = collection_requests_.begin();
    while(iter != collection_requests_.end())
    {
      auto const &key = iter->first;
      auto const &promise = iter->second;

      switch (promise.GetState())
      {
        case PromiseState::WAITING:
          ++iter;
          break;

        case PromiseState::SUCCESS:
        {
          ContainerOfPromised const results = promise.Get();

          for (auto const &promised : results)
          {
            completed_.emplace_back(Result{key, promised});
          }

          iter = collection_requests_.erase(iter);
          break;
        }

        case PromiseState::FAILED:
          iter = collection_requests_.erase(iter);
          break;
      }
    }
  }

  {
    auto iter = single_requests_.begin();
    while(iter != single_requests_.end())
    {
      auto const &key = iter->first;
      auto const &promise = iter->second;

      switch(promise.GetState())
      {
        case PromiseState::WAITING:
          ++iter;
          break;
        case PromiseState::SUCCESS:
          completed_.emplace_back(Result{key, promise.Get()});
          iter = single_requests_.erase(iter);
          break;
        case PromiseState::FAILED:
          iter = single_requests_.erase(iter);
          break;
      }
    }
  }
}

template<typename K, typename P, typename C>
typename RequestingQueueOf<K,P,C>::KeySet RequestingQueueOf<K,P,C>::FilterOutInFlight(KeySet const &inputs)
{
  FETCH_LOCK(mutex_);

  KeySet keys;

  std::copy_if(
    inputs.begin(),
    inputs.end(),
    std::inserter(keys, keys.begin()),
    [this](Key const &key)
    {
      bool const is_not_collection_key = collection_requests_.find(key) == collection_requests_.end();
      bool const is_not_single_key = single_requests_.find(key) == single_requests_.end();

      return (is_not_collection_key && is_not_single_key);
    }
  );

  return keys;
}

template<typename K, typename P, typename C>
bool RequestingQueueOf<K,P,C>::IsInFlight(Key const &key) const
{
  FETCH_LOCK(mutex_);

  // check if the key is present in any of the requests maps
  bool const is_a_collection_key = collection_requests_.find(key) != collection_requests_.end();
  bool const is_a_single_key = single_requests_.find(key) != single_requests_.end();

  return (is_a_collection_key || is_a_single_key);
}

#if 0
template<typename K, typename P, typename C>
std::size_t RequestingQueueOf<K,P,C>::size() const
{
  FETCH_LOCK(mutex_);
  return collection_requests_.size() + single_requests_.size();
}
#endif

template<typename K, typename P, typename C>
bool RequestingQueueOf<K,P,C>::HasCompletedPromises() const
{
  FETCH_LOCK(mutex_);
  return !completed_.empty();
}

}  // namespace network
}  // namespace fetch
