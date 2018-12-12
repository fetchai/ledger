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

#include "core/serializers/stl_types.hpp"
#include "network/generics/promise_of.hpp"
#include "network/service/promise.hpp"

#include <algorithm>
#include <atomic>
#include <deque>
#include <iterator>
#include <unordered_map>

namespace fetch {
namespace network {

template <typename K, typename R, typename P = PromiseOf<R>>
class RequestingQueueOf
{
public:
  using Key          = K;
  using Promised     = R;
  using Mutex        = fetch::mutex::Mutex;
  using Lock         = std::lock_guard<Mutex>;
  using Promise      = P;
  using PromiseState = fetch::service::PromiseState;
  using PromiseMap   = std::unordered_map<Key, Promise>;
  using KeySet       = std::unordered_set<Key>;
  using Counter      = std::atomic<std::size_t>;
  using Timepoint    = typename Promise::Timepoint;

  struct SuccessfulResult
  {
    Key      key;
    Promised promised;
  };

  struct FailedResult
  {
    Key     key;
    Promise promise;
  };

  struct Counters
  {
    std::size_t completed;
    std::size_t failed;
    std::size_t pending;
  };

  using SuccessfulResults = std::deque<SuccessfulResult>;
  using FailedResults     = std::deque<FailedResult>;

  // Construction / Destruction
  RequestingQueueOf()                          = default;
  RequestingQueueOf(RequestingQueueOf const &) = delete;
  RequestingQueueOf(RequestingQueueOf &&)      = delete;
  ~RequestingQueueOf()                         = default;

  bool Add(Key const &key, Promise const &promise);

  SuccessfulResults Get(std::size_t limit);
  FailedResults     GetFailures(std::size_t limit);
  PromiseMap        GetPending();

  KeySet FilterOutInFlight(KeySet const &inputs);
  bool   IsInFlight(Key const &key) const;

  Counters Resolve();
  Counters Resolve(Timepoint const &time_point);

  bool HasCompletedPromises() const;
  bool HasFailedPromises() const;
  void DiscardFailures();
  void DiscardCompleted();

  // Operators
  RequestingQueueOf &operator=(RequestingQueueOf const &) = delete;
  RequestingQueueOf &operator=(RequestingQueueOf &&) = delete;

private:
  mutable Mutex     mutex_{__LINE__, __FILE__};
  PromiseMap        requests_;   ///< The map of currently monitored promises
  SuccessfulResults completed_;  ///< The map of completed promises
  FailedResults     failed_;     ///< The set of failed keys
  Counter           num_completed_{0};
  Counter           num_failed_{0};
  Counter           num_pending_{0};
};

template <typename K, typename R, typename P>
bool RequestingQueueOf<K, R, P>::Add(Key const &key, Promise const &promise)
{
  bool success = false;

  {
    FETCH_LOCK(mutex_);

    if (requests_.find(key) == requests_.end())
    {
      requests_.emplace(key, promise);
      ++num_pending_;
      success = true;
    }
  }

  return success;
}

template <typename K, typename R, typename P>
typename RequestingQueueOf<K, R, P>::SuccessfulResults RequestingQueueOf<K, R, P>::Get(
    std::size_t limit)
{
  FETCH_LOCK(mutex_);

  SuccessfulResults results;

  if (limit == 0)
  {
    assert(false);
  }
  else if (completed_.size() <= limit)
  {
    results = std::move(completed_);
    completed_.clear();  // needed?
  }
  else  // (completed_.size() > limit)
  {
    // copy "limit" number of entries from the completed list
    std::copy_n(completed_.begin(), limit, std::inserter(results, results.begin()));

    // erase these entries from the completed map
    completed_.erase(completed_.begin(), completed_.begin() + static_cast<std::ptrdiff_t>(limit));
  }

  return results;
}

template <typename K, typename R, typename P>
typename RequestingQueueOf<K, R, P>::FailedResults RequestingQueueOf<K, R, P>::GetFailures(
    std::size_t limit)
{
  FETCH_LOCK(mutex_);

  FailedResults results;

  if (limit == 0)
  {
    assert(false);
  }
  else if (failed_.size() <= limit)
  {
    results = std::move(failed_);
    failed_.clear();  // needed?
  }
  else  // (completed_.size() > limit)
  {
    // copy "limit" number of entries from the completed list
    std::copy_n(failed_.begin(), limit, std::inserter(results, results.begin()));

    // erase these entries from the completed map
    failed_.erase(failed_.begin(), failed_.begin() + static_cast<std::ptrdiff_t>(limit));
  }

  return results;
}

template <typename K, typename R, typename P>
typename RequestingQueueOf<K, R, P>::PromiseMap RequestingQueueOf<K, R, P>::GetPending()
{
  FETCH_LOCK(mutex_);
  PromiseMap pending = std::move(requests_);
  requests_.clear();
  return pending;
}

template <typename K, typename R, typename P>
typename RequestingQueueOf<K, R, P>::Counters RequestingQueueOf<K, R, P>::Resolve()
{
  FETCH_LOCK(mutex_);

  auto iter = requests_.begin();
  while (iter != requests_.end())
  {
    auto const &key     = iter->first;
    auto &      promise = iter->second;

    switch (promise.GetState())
    {
    case PromiseState::WAITING:
      ++iter;
      break;
    case PromiseState::SUCCESS:
      completed_.emplace_back(SuccessfulResult{key, promise.Get()});
      ++num_completed_;
      --num_pending_;
      iter = requests_.erase(iter);
      break;
    case PromiseState::FAILED:
    case PromiseState::TIMEDOUT:
      failed_.emplace_back(FailedResult{key, promise});
      ++num_failed_;
      --num_pending_;
      iter = requests_.erase(iter);
      break;
    default:
      break;
    }
  }

  // TODO(EJF): Not really sure why the value is here?
  num_completed_ = completed_.size();
  num_pending_   = requests_.size();
  num_failed_    = failed_.size();

  return {completed_.size(), failed_.size(), requests_.size()};
}

template <typename K, typename R, typename P>
typename RequestingQueueOf<K, R, P>::Counters RequestingQueueOf<K, R, P>::Resolve(
    Timepoint const &time_point)
{
  FETCH_LOCK(mutex_);

  auto iter = requests_.begin();
  while (iter != requests_.end())
  {
    auto const &key     = iter->first;
    auto &      promise = iter->second;

    switch (promise.GetState(time_point))
    {
    case PromiseState::WAITING:
      ++iter;
      break;
    case PromiseState::SUCCESS:
      completed_.emplace_back(SuccessfulResult{key, promise.Get()});
      ++num_completed_;
      --num_pending_;
      iter = requests_.erase(iter);
      break;
    case PromiseState::FAILED:
    case PromiseState::TIMEDOUT:
      failed_.emplace_back(FailedResult{key, promise});
      ++num_failed_;
      --num_pending_;
      iter = requests_.erase(iter);
      break;
    default:
      break;
    }
  }

  // TODO(EJF): Not really sure why the value is here?
  num_completed_ = completed_.size();
  num_pending_   = requests_.size();
  num_failed_    = failed_.size();

  return {completed_.size(), failed_.size(), requests_.size()};
}

template <typename K, typename R, typename P>
typename RequestingQueueOf<K, R, P>::KeySet RequestingQueueOf<K, R, P>::FilterOutInFlight(
    KeySet const &inputs)
{
  FETCH_LOCK(mutex_);

  KeySet keys;

  std::copy_if(inputs.begin(), inputs.end(), std::inserter(keys, keys.begin()),
               [this](Key const &key) { return requests_.find(key) == requests_.end(); });

  return keys;
}

template <typename K, typename R, typename P>
bool RequestingQueueOf<K, R, P>::IsInFlight(Key const &key) const
{
  FETCH_LOCK(mutex_);
  return requests_.find(key) != requests_.end();
}

template <typename K, typename R, typename P>
bool RequestingQueueOf<K, R, P>::HasCompletedPromises() const
{
  FETCH_LOCK(mutex_);
  return !completed_.empty();
}

template <typename K, typename R, typename P>
bool RequestingQueueOf<K, R, P>::HasFailedPromises() const
{
  FETCH_LOCK(mutex_);
  return !failed_.empty();
}

template <typename K, typename R, typename P>
void RequestingQueueOf<K, R, P>::DiscardFailures()
{
  FETCH_LOCK(mutex_);
  failed_.clear();
}

template <typename K, typename R, typename P>
void RequestingQueueOf<K, R, P>::DiscardCompleted()
{
  FETCH_LOCK(mutex_);
  completed_.clear();
}

}  // namespace network
}  // namespace fetch
