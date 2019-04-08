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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <mutex>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <core/mutex.hpp>
#include <meta/type_util.hpp>
#include <storage/key_value_index.hpp>

namespace fetch {
namespace generics {

namespace black {

using Clock     = std::chrono::system_clock;
using Timepoint = Clock::time_point;

template <class Set, class Key>
inline typename Set::const_iterator FindQuarantined(Set &suspended, Key const &key)
{
  auto where{suspended.find(key)};
  if (where != suspended.cend())
  {
    if (where->second > Clock::now())
    {
      return where;
    }
    suspended.erase(where);
  }
  return suspended.cend();
}

template <class Set, class Key>
inline bool Quarantined(Set &suspended, Key const &key)
{
  return FindQuarantined(suspended, key) != suspended.cend();
}

template <class Persistence>
class Cache;
template <class T>
class Persistence;
template <class T>
class NoPersistence;

}  // namespace black

template <class Cache, class Mx = mutex::Mutex>
class BasicBlackset
{
public:
  using Mutex     = Mx;
  using Value     = typename Cache::Value;
  using Clock     = typename Cache::Clock;
  using Timepoint = typename Cache::Timepoint;

  template <typename... Args>
  BasicBlackset(Mutex &mutex, Args &&... args)
    : mutex_(&mutex)
    , cache_(std::forward<Args>(args)...)
  {}
  BasicBlackset(BasicBlackset const &) = default;
  BasicBlackset(BasicBlackset &&)      = default;

  BasicBlackset &operator=(BasicBlackset const &)    = default;
  BasicBlackset &operator                            =(BasicBlackset &&) noexcept(
      std::is_nothrow_move_assignable<Cache>::value) = default;

  template <typename... Args>
  void Blacklist(Args &&... args)
  {
    FETCH_LOCK(*mutex_);
    LocklessBlacklist(std::forward<Args>(args)...);
  }

  void Quarantine(Timepoint until, Value t)
  {
    FETCH_LOCK(*mutex_);
    LocklessQuarantine(until, std::move(t));
  }
  template <typename Begin, typename End>
  void Quarantine(Timepoint const &until, Begin begin, End end)
  {
    FETCH_LOCK(*mutex_);
    LocklessQuarantine(until, std::move(begin), std::move(end));
  }

  bool IsBlacklisted(Value const &t) const
  {
    FETCH_LOCK(*mutex_);
    return LocklessIsBlacklisted(t);
  }
  auto GetBlacklisted() const
  {
    FETCH_LOCK(*mutex_);
    return LocklessGetBlacklisted();
  }
  auto GetQuarantined() const
  {
    FETCH_LOCK(*mutex_);
    return LocklessGetQuarantined();
  }

  void Whitelist(Value const &t)
  {
    FETCH_LOCK(*mutex_);
    LocklessWhitelist(t);
  }
  void Whitelist(std::initializer_list<Value> ts)
  {
    Whitelist(ts.begin(), ts.end());
  }
  template <typename Begin, typename End>
  void Whitelist(Begin begin, End end)
  {
    FETCH_LOCK(*mutex_);
    LocklessWhitelist(std::move(begin), std::move(end));
  }

protected:
  template <typename... Args>
  void LocklessBlacklist(Args &&... args)
  {
    cache_.Blacklist(std::forward<Args>(args)...);
  }

  void LocklessQuarantine(Timepoint until, Value t)
  {
    cache_.Quarantine(until, std::move(t));
  }
  template <typename Begin, typename End>
  void LocklessQuarantine(Timepoint const &until, Begin begin, End end)
  {
    cache_.Quarantine(until, std::move(begin), std::move(end));
  }

  bool LocklessIsBlacklisted(Value const &t) const
  {
    return cache_.IsBlacklisted(t);
  }
  auto LocklessGetBlacklisted() const
  {
    return cache_.GetBlacklisted();
  }
  auto LocklessGetQuarantined() const
  {
    return cache_.GetQuarantined();
  }

  void LocklessWhitelist(Value const &t)
  {
    cache_.Whitelist(t);
  }
  void LocklessWhitelist(std::initializer_list<Value> ts)
  {
    LocklessWhitelist(ts.begin(), ts.end());
  }
  template <typename Begin, typename End>
  void LocklessWhitelist(Begin begin, End end)
  {
    cache_.Whitelist(std::move(begin), std::move(end));
  }

private:
  Mx *  mutex_;
  Cache cache_;
};

template <typename T, class Mutex = mutex::Mutex>
using Blackset = BasicBlackset<black::Cache<black::NoPersistence<T>>, Mutex>;
template <typename T>
using UnguardedBlackset = BasicBlackset<black::Cache<black::NoPersistence<T>>, void>;
template <typename T, class Mutex = mutex::Mutex>
using PersistentBlackset = BasicBlackset<black::Cache<black::Persistence<T>>, Mutex>;
template <typename T>
using UnguardedPersistentBlackset = BasicBlackset<black::Cache<black::Persistence<T>>, void>;

template <class Cache>
class BasicBlackset<Cache, void>
{
public:
  using Mutex     = void;
  using Value     = typename Cache::Value;
  using Clock     = typename Cache::Clock;
  using Timepoint = typename Cache::Timepoint;

  template <typename... Args>
  BasicBlackset(Args &&... args)
    : cache_(std::forward<Args>(args)...)
  {}
  BasicBlackset(BasicBlackset const &) = default;
  BasicBlackset(BasicBlackset &&)      = default;

  BasicBlackset &operator=(BasicBlackset const &)    = default;
  BasicBlackset &operator                            =(BasicBlackset &&) noexcept(
      std::is_nothrow_move_assignable<Cache>::value) = default;

  template <typename... Args>
  void Blacklist(Args &&... args)
  {
    LocklessBlacklist(std::forward<Args>(args)...);
  }

  void Quarantine(Timepoint until, Value t)
  {
    LocklessQuarantine(until, std::move(t));
  }
  template <typename Begin, typename End>
  void Quarantine(Timepoint const &until, Begin begin, End end)
  {
    LocklessQuarantine(until, std::move(begin), std::move(end));
  }

  bool IsBlacklisted(Value const &t) const
  {
    return LocklessIsBlacklisted(t);
  }
  auto GetBlacklisted() const
  {
    return LocklessGetBlacklisted();
  }
  auto GetQuarantined() const
  {
    return LocklessGetQuarantined();
  }

  void Whitelist(Value const &t)
  {
    LocklessWhitelist(t);
  }
  void Whitelist(std::initializer_list<Value> ts)
  {
    Whitelist(ts.begin(), ts.end());
  }
  template <typename Begin, typename End>
  void Whitelist(Begin begin, End end)
  {
    LocklessWhitelist(std::move(begin), std::move(end));
  }

protected:
  template <typename... Args>
  void LocklessBlacklist(Args &&... args)
  {
    cache_.Blacklist(std::forward<Args>(args)...);
  }

  void LocklessQuarantine(Timepoint until, Value t)
  {
    cache_.Quarantine(until, std::move(t));
  }
  template <typename Begin, typename End>
  void LocklessQuarantine(Timepoint const &until, Begin begin, End end)
  {
    cache_.Quarantine(until, std::move(begin), std::move(end));
  }

  bool LocklessIsBlacklisted(Value const &t) const
  {
    return cache_.IsBlacklisted(t);
  }
  auto LocklessGetBlacklisted() const
  {
    return cache_.GetBlacklisted();
  }
  auto LocklessGetQuarantined() const
  {
    return cache_.GetQuarantined();
  }

  void LocklessWhitelist(Value const &t)
  {
    cache_.Whitelist(t);
  }
  void LocklessWhitelist(std::initializer_list<Value> ts)
  {
    LocklessWhitelist(ts.begin(), ts.end());
  }
  template <typename Begin, typename End>
  void LocklessWhitelist(Begin begin, End end)
  {
    cache_.Whitelist(std::move(begin), std::move(end));
  }

private:
  Cache cache_;
};

namespace black {

template <class Persistence>
class Cache
{
public:
  using Value     = typename Persistence::Value;
  using Clock     = black::Clock;
  using Timepoint = black::Timepoint;

private:
  using Banned    = typename Persistence::Banned;
  using Suspended = typename Persistence::Suspended;

public:
  template <typename... Args>
  Cache(Args &&... args) noexcept(
      std::is_nothrow_constructible<Persistence, Args...>::value &&noexcept(
          std::declval<Persistence &>().Populate(std::declval<Banned &>(),
                                                 std::declval<Suspended &>())))
    : persistence_(std::forward<Args>(args)...)
  {
    persistence_.Populate(banned_, suspended_);
  }
  Cache(Cache &&) noexcept(type_util::AllV<std::is_nothrow_move_constructible, Persistence, Banned,
                                           Suspended>) = default;

  Cache &operator=(Cache &&) noexcept(
      type_util::AllV<std::is_nothrow_move_assignable, Persistence, Banned, Suspended>) = default;

  template <typename... Args>
  void Blacklist(Args &&... args)
  {
    persistence_.Blacklist(args...);
    banned_.insert(std::forward<Args>(args)...);
  }

  void Quarantine(Timepoint until, Value t)
  {
    persistence_.Quarantine(until, t);
    suspended_.emplace(std::move(t), std::move(until));
  }
  template <typename Begin, typename End>
  void Quarantine(Timepoint until, Begin begin, End end)
  {
    while (begin != end)
    {
      persistence_.Quarantine(until, static_cast<decltype(*begin) &>(*begin));
      suspended_.emplace(*begin, until);
      ++begin;
    }
  }

  bool IsBlacklisted(Value const &t) const
  {
    return banned_.find(t) != banned_.end() || black::Quarantined(suspended_, t);
  }
  auto GetBlacklisted() const
  {
    return banned_;
  }
  auto GetQuarantined() const
  {
    return suspended_;
  }

  void Whitelist(Value const &t)
  {
    persistence_.Whitelist(t);
    banned_.erase(t) || suspended_.erase(t);
  }
  void Whitelist(std::initializer_list<Value> ts)
  {
    Whitelist(ts.begin(), ts.end());
  }
  template <typename Begin, typename End>
  void Whitelist(Begin begin, End end)
  {
    while (begin != end)
    {
      persistence_.Whitelist(*begin);
      banned_.erase(*begin) || suspended_.erase(*begin);
      ++begin;
    }
  }

private:
  Persistence       persistence_;
  Banned            banned_;
  mutable Suspended suspended_;
};

template <class T>
class NoPersistence
{
public:
  using Value     = T;
  using Clock     = black::Clock;
  using Timepoint = black::Timepoint;
  using Banned    = std::unordered_set<T>;
  using Suspended = std::unordered_map<T, Clock::time_point>;

  template <typename... Args>
  NoPersistence(Args &&... args) noexcept(std::is_nothrow_constructible<Banned, Args...>::value)
    : banned_(std::forward<Args>(args)...)
  {}

  void Populate(Banned &banned, Suspended &)
      noexcept(noexcept(std::declval<Banned &>().swap(std::declval<Banned &>())))
  {
    banned.swap(banned_);
  }

  template <typename... Args>
  static void Blacklist(Args &&...) noexcept
  {}

  template <typename... Args>
  static void Quarantine(Args &&...) noexcept
  {}

  template <typename... Args>
  static void Whitelist(Args &&...) noexcept
  {}

private:
  Banned banned_;
};

template <class T>
class Persistence
{
  using Implementation = fetch::storage::KeyValueIndex<>;

  using StoredType              = uint64_t;
  static constexpr auto forever = std::numeric_limits<StoredType>::max();
  // TODO: Implementation::Delete() currently not implemented,
  // this value signifies a whitelisted entry
  static constexpr auto never = std::numeric_limits<StoredType>::min();

public:
  using Value     = T;
  using Clock     = black::Clock;
  using Timepoint = black::Timepoint;
  using Banned    = std::unordered_set<T>;
  using Suspended = std::unordered_map<T, Clock::time_point>;

  Persistence() = default;
  Persistence(std::string filename, std::size_t flushingThreshold = DEFAULT_THRESHOLD)
    : threshold_(flushingThreshold)
    , file_(std::make_unique<Implementation>())
    , synchronization_(run())
  {
    assert(!filename.empty());
    file_->Load(std::move(filename), true);
  }
  Persistence(Persistence &&) = default;

  ~Persistence()
  {
    stop();
  }

  void Populate(Banned &banned, Suspended &suspended)
  {
    if (!valid())
    {
      return;
    }
    Timepoint now{Clock::now()};
    for (auto const &kv : *file_)
    {
      if (kv.second == forever)
      {
        banned.insert(kv.first);
      }
      else
      {
        Timepoint then{Clock::from_time_t(static_cast<std::time_t>(kv.second))};
        if (then > now)
        {
          suspended.emplace(kv.first, then);
        }
      }
    }
  }

  template <typename... Args>
  void Blacklist(Args &&... args)
  {
    Set(T(std::forward<Args>(args)...), forever);
  }

  template <typename... Args>
  void Quarantine(Timepoint until, Args &&... args)
  {
    Set(T(std::forward<Args>(args)...), static_cast<StoredType>(Clock::to_time_t(until)));
  }

  template <typename... Args>
  void Whitelist(Args &&... args)
  {
    Set(T(std::forward<Args>(args)...), never);
  }

private:
  bool valid() const noexcept
  {
    return bool{file_};
  }

  void Set(T t, StoredType until)
  {
    if (valid())
    {
      static const byte_array::ConstByteArray noData;
      file_->Set(byte_array::ToConstByteArray(std::forward<T>(t)), until, noData);
      if (++mutations_ >= threshold_)
      {
        flush();
        mutations_ = 0;
      }
    }
  }

  void stop() noexcept
  {
    if (valid())
    {
      running_ = false;
      flush();
      synchronization_.join();
    }
  }

  void flush() noexcept
  {
    flush_signal_.notify_one();
  }

  auto run()
  {
    return std::thread([this]() {
      try
      {
        while (running_)
        {
          std::unique_lock<Mutex> lock(flush_mutex_);
          flush_signal_.wait(lock);
          file_->Flush();
        }
      }
      catch (...)
      {
        try
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Blackset is not flushed");
        }
        catch (...)
        {
        }
      }
    });
  }

  using Mutex = std::mutex;

  static constexpr std::size_t DEFAULT_THRESHOLD = 16;
  static constexpr char const *LOGGING_NAME      = "Persistence";

  const std::size_t threshold_ = DEFAULT_THRESHOLD;
  std::size_t       mutations_ = 0;

  std::unique_ptr<Implementation> file_;

  std::condition_variable flush_signal_;
  Mutex                   flush_mutex_;
  std::thread             synchronization_;
  std::atomic_bool        running_{true};
};

}  // namespace black
}  // namespace generics
}  // namespace fetch
