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

#include <chrono>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>
#include <utility>

#include <core/mutex.hpp>
#include <core/type_util.hpp>
#include <storage/key_value_index.hpp>

namespace fetch {
namespace generics {

namespace detail_ {
  using Clock     = std::chrono::system_clock;
  using Timepoint = Clock::time_point;

  template<class Set, class Key>
  inline typename Set::const_iterator FindQuarantined(Set &suspended, Key const &key)
  {
    auto where{suspended.find(key)};
    if(where != suspended.cend())
    {
      if(where->second > Clock::now())
      {
        return where;
      }
      suspended.erase(where);
    }
    return suspended.cend();
  }

  template<class Set, class Key>
  inline bool Quarantined(Set &suspended, Key const &key)
  {
    return FindQuarantined(suspended, key) != suspended.cend();
  }
}


namespace black {

template <class Persistence> class Cache;
template <class T> class Persistence;
template <class T> class NoPersistence;

}


template <class Cache, class Mx = mutex::Mutex>
class BasicBlackset
{
public:
  using Mutex     = Mx;
  using Value     = typename Cache::Value;
  using Clock     = typename Cache::Clock;
  using Timepoint = typename Cache::Timepoint;


  template<typename... Args>
  Blackset(mutex_type &mutex, Args &&...args)
    : mutex_(&mutex)
    , cache_(std::forward<Args>(args)...)
  {}
  Blackset(Blackset const &) = default;
  Blackset(Blackset &&)      = default;


  Blackset &operator=(Blackset const &) = default;
  Blackset &operator=(Blackset &&) noexcept(std::is_nothrow_move_assignable<Cache>::value) = default;


  template <typename... Args>
  void Blacklist(Args &&... args)
  {
    FETCH_LOCK(*mutex_);
    cache_.Blacklist(std::forward<Args>(args)...);
  }


  void Quarantine(Timepoint until, Value t)
  {
    FETCH_LOCK(*mutex_);
    cache_.Quarantine(until, std::move(t));
  }
  template<typename Begin, typename End>
  void Quarantine(Timepoint const &until, Begin begin, End end)
  {
    FETCH_LOCK(*mutex_);
    cache_.Quarantine(until, std::move(begin), std::move(end));
  }


  bool IsBlacklisted(ValueType const &t) const
  {
    FETCH_LOCK(*mutex_);
    return cache_.IsBlacklisted(t);
  }
  auto GetBlacklisted() const
  {
    FETCH_LOCK(*mutex_);
    return cache_.GetBlackListed();
  }
  auto GetQuarantined() const
  {
    FETCH_LOCK(*mutex_);
    return cache_.GetQuarantined();
  }


  void Whitelist(ValueType const &t)
  {
    FETCH_LOCK(*mutex_);
    cache_.Whitelist(t);
  }
  void Whitelist(std::initializer_list<ValueType> ts)
  {
    Whitelist(ts.begin(), ts.end());
  }
  template<typename Begin, typename End>
  void Whitelist(Begin begin, End end)
  {
    FETCH_LOCK(*mutex_);
    cache_.Whitelist(std::move(begin), std::move(end));
  }

private:
  Mx * mutex_;
  Cache cache_;
};


template<typename T, class Mutex = mutex::Mutex> using Blackset                    = BasicBlackset<black::Cache<black::NoPersistence<T>>, Mutex>;
template<typename T>                             using UnguardedBlackset           = BasicBlackset<black::Cache<black::NoPersistence<T>>, void>;
template<typename T, class Mutex = mutex::Mutex> using PersistentBlackset          = BasicBlackset<black::Cache<black::Persistence<T>>, Mutex>;
template<typename T>                             using UnguardedPersistentBlackset = BasicBlackset<black::Cache<black::Persistence<T>>, void>;


template <class Cache> class BasicBlackset<Cache, void>
{
public:
  using Mutex     = void;
  using Value     = typename Cache::Value;
  using Clock     = typename Cache::Clock;
  using Timepoint = typename Cache::Timepoint;


  template<typename... Args>
  Blackset(Args &&...args)
    : cache_(std::forward<Args>(args)...)
  {}
  Blackset(Blackset const &) = default;
  Blackset(Blackset &&)      = default;


  Blackset &operator=(Blackset const &) = default;
  Blackset &operator=(Blackset &&) noexcept(std::is_nothrow_move_assignable<Cache>::value) = default;


  template <typename... Args>
  void Blacklist(Args &&... args)
  {
    cache_.Blacklist(std::forward<Args>(args)...);
  }


  void Quarantine(Timepoint until, Value t)
  {
    cache_.Quarantine(until, std::move(t));
  }
  template<typename Begin, typename End>
  void Quarantine(Timepoint const &until, Begin begin, End end)
  {
    cache_.Quarantine(until, std::move(begin), std::move(end));
  }


  bool IsBlacklisted(ValueType const &t) const
  {
    return cache_.IsBlacklisted(t);
  }
  auto GetBlacklisted() const
  {
    return cache_.GetBlackListed();
  }
  auto GetQuarantined() const
  {
    return cache_.GetQuarantined();
  }


  void Whitelist(ValueType const &t)
  {
    cache_.Whitelist(t);
  }
  void Whitelist(std::initializer_list<ValueType> ts)
  {
    Whitelist(ts.begin(), ts.end());
  }
  template<typename Begin, typename End>
  void Whitelist(Begin begin, End end)
  {
    cache_.Whitelist(std::move(begin), std::move(end));
  }

private:
  Cache cache_;
};


// It assumes blacklisted values are coupled, and the first one is primary.
template <class T1, class T2, class Mx = mutex::Mutex> class Blackset2
{
public:
  using mutex_type = Mx;
  using ValueType1 = T1;
  using ValueType2 = T2;
  using Clock = detail_::Clock;
  using Timepoint  = Clock::time_point;

private:
  using Banned1 = std::unordered_set<T1>;
  using Banned2 = std::unordered_set<T2>;
  using Suspended1 = std::unordered_map<T1, Clock::time_point>;
  using Suspended2 = std::unordered_map<T2, Clock::time_point>;

public:
  Blackset2(mutex_type &mutex)
    : mutex_(&mutex)
  {}
  Blackset2(Blackset2 const &) = default;
  Blackset2(Blackset2 &&)      = default;


  Blackset2 &operator=(Blackset2 const &) = default;
  Blackset2 &operator=(Blackset2 &&) noexcept(std::is_nothrow_move_assignable<Banned1>::value && std::is_nothrow_move_assignable<Banned2>::value) = default;


  Blackset2 &Blacklist1(ValueType1 t1) {
	  FETCH_LOCK(*mutex_);
	  banned1_.insert(std::move(t1));
	  return *this;
  }
  Blackset2 &Blacklist2(ValueType2 t2) {
	  FETCH_LOCK(*mutex_);
	  banned2_.insert(std::move(t2));
	  return *this;
  }
  Blackset2 &Blacklist(ValueType1 t1, ValueType2 t2) {
	  FETCH_LOCK(*mutex_);
	  banned1_.insert(std::move(t1));
	  banned2_.insert(std::move(t2));
	  return *this;
  }


  Blackset2 &Quarantine1(Timepoint until, ValueType1 t1)
  {
    FETCH_LOCK(*mutex_);
    suspended1_.emplace(std::move(t1), std::move(until));
    return *this;
  }
  Blackset2 &Quarantine2(Timepoint until, ValueType2 t2)
  {
    FETCH_LOCK(*mutex_);
    suspended2_.emplace(std::move(t2), std::move(until));
    return *this;
  }
  Blackset2 &Quarantine(Timepoint until, ValueType1 t1, ValueType2 t2)
  {
    FETCH_LOCK(*mutex_);
    suspended1_.emplace(std::move(t1), until);
    suspended2_.emplace(std::move(t2), std::move(until));
    return *this;
  }


  bool IsBlacklisted1(ValueType1 const &t1) const
  {
    FETCH_LOCK(*mutex_);
    return Blacklisted1(t1);
  }
  bool IsBlacklisted2(ValueType2 const &t2) const
  {
    FETCH_LOCK(*mutex_);
    return Blacklisted2(t2);
  }
  bool IsBlacklisted(ValueType1 const &t1, ValueType2 const &t2) const
  {
    FETCH_LOCK(*mutex_);
    return Blacklisted(t1, t2);
  }


  Blackset2 &Whitelist1(ValueType1 const &t1)
  {
    FETCH_LOCK(*mutex_);
    banned1_.erase(t1) || suspended1_.erase(t1);
    return *this;
  }
  Blackset2 &Whitelist2(ValueType2 const &t2)
  {
    FETCH_LOCK(*mutex_);
    banned2_.erase(t2) || suspended2_.erase(t2);
    return *this;
  }
  Blackset2 &Whitelist(ValueType1 const &t1, ValueType2 const &t2)
  {
    FETCH_LOCK(*mutex_);
    banned1_.erase(t1) || suspended1_.erase(t1);
    banned2_.erase(t2) || suspended2_.erase(t2);
    return *this;
  }

protected:
  bool Blacklisted1(ValueType1 const &t) const
  {
    return banned1_.find(t) != banned1_.end() || detail_::Quarantined(suspended1_, t);
  }
  bool Blacklisted2(ValueType2 const &t) const
  {
    return banned2_.find(t) != banned2_.end() || detail_::Quarantined(suspended2_, t);
  }
  bool Blacklisted(ValueType1 const &t1, ValueType2 const &t2) const
  {
    if(banned1_.find(t1) != banned1_.end() || detail_::Quarantined(suspended1_, t1)) return true;
    if(banned2_.find(t2) != banned2_.end()) {
	    banned1_.insert(t1);
	    return true;
    }
    ConstIterator2 where{detail_::FindQuarantined(suspended2_, t2)};
    if(where != suspended2_.end()) {
	    suspended1_.emplace(t1, where->second);
	    return true;
    }
    return false;
  }
private:
  using ConstIterator2 = typename Suspended2::const_iterator;

  Mx * mutex_;
  mutable Banned1 banned1_;
  Banned2 banned2_;
  mutable Suspended1 suspended1_;
  mutable Suspended2 suspended2_;
};

template <class T1, class T2>
class Blackset2<T1, T2, void>
{
public:
  using mutex_type = void;
  using ValueType1 = T1;
  using ValueType2 = T2;
  using Clock = detail_::Clock;
  using Timepoint  = Clock::time_point;

private:
  using Banned1 = std::unordered_set<T1>;
  using Banned2 = std::unordered_set<T2>;
  using Suspended1 = std::unordered_map<ValueType1, Clock::time_point>;
  using Suspended2 = std::unordered_map<ValueType2, Clock::time_point>;

public:
  Blackset2() = default;
  Blackset2(Blackset2 const &) = default;
  Blackset2(Blackset2 &&)      = default;


  Blackset2 &operator=(Blackset2 const &) = default;
  Blackset2 &operator=(Blackset2 &&) noexcept(std::is_nothrow_move_assignable<Banned1>::value && std::is_nothrow_move_assignable<Banned2>::value) = default;


  Blackset2 &Blacklist1(ValueType1 t1) {
	  banned1_.insert(std::move(t1));
	  return *this;
  }
  Blackset2 &Blacklist2(ValueType2 t2) {
	  banned2_.insert(std::move(t2));
	  return *this;
  }
  Blackset2 &Blacklist(ValueType1 t1, ValueType2 t2) {
	  banned1_.insert(std::move(t1));
	  banned2_.insert(std::move(t2));
	  return *this;
  }


  Blackset2 &Quarantine1(Timepoint until, ValueType1 t1)
  {
    suspended1_.emplace(std::move(t1), std::move(until));
    return *this;
  }
  Blackset2 &Quarantine2(Timepoint until, ValueType2 t2)
  {
    suspended2_.emplace(std::move(t2), std::move(until));
    return *this;
  }
  Blackset2 &Quarantine(Timepoint until, ValueType1 t1, ValueType2 t2)
  {
    suspended1_.emplace(std::move(t1), until);
    suspended2_.emplace(std::move(t2), std::move(until));
    return *this;
  }


  bool IsBlacklisted1(ValueType1 const &t1) const
  {
    return Blacklisted1(t1);
  }
  bool IsBlacklisted2(ValueType2 const &t2) const
  {
    return Blacklisted2(t2);
  }
  bool IsBlacklisted(ValueType1 const &t1, ValueType2 const &t2) const
  {
    return Blacklisted(t1, t2);
  }


  Blackset2 &Whitelist1(ValueType1 const &t1)
  {
    banned1_.erase(t1) || suspended1_.erase(t1);
    return *this;
  }
  Blackset2 &Whitelist2(ValueType2 const &t2)
  {
    banned2_.erase(t2) || suspended2_.erase(t2);
    return *this;
  }
  Blackset2 &Whitelist(ValueType1 const &t1, ValueType2 const &t2)
  {
    banned1_.erase(t1) || suspended1_.erase(t1);
    banned2_.erase(t2) || suspended2_.erase(t2);
    return *this;
  }

protected:
  bool Blacklisted1(ValueType1 const &t) const
  {
    return banned1_.find(t) != banned1_.end() || detail_::Quarantined(suspended1_, t);
  }
  bool Blacklisted2(ValueType2 const &t) const
  {
    return banned2_.find(t) != banned2_.end() || detail_::Quarantined(suspended2_, t);
  }
  bool Blacklisted(ValueType1 const &t1, ValueType2 const &t2) const
  {
    if(banned1_.find(t1) != banned1_.end() || detail_::Quarantined(suspended1_, t1)) return true;
    if(banned2_.find(t2) != banned2_.end())
    {
      banned1_.insert(t1);
      return true;
    }
    ConstIterator2 where{detail_::FindQuarantined(suspended2_, t2)};
    if(where != suspended2_.end())
    {
      suspended1_.emplace(t1, where->second);
      return true;
    }
    return false;
  }
private:
  using ConstIterator2 = typename Suspended2::const_iterator;

  mutable Banned1 banned1_;
  Banned2 banned2_;
  mutable Suspended1 suspended1_;
  mutable Suspended2 suspended2_;
};


namespace black {


template <class Persistence>
class Cache
{
public:
  using Value     = typename Persistence::Value;
  using Clock     = detail_::Clock;
  using Timepoint = detail_::Timepoint;

private:
  using Banned    = typename Persistence::Banned;
  using Suspended = typename Persistence::Suspended;

public:
  template<typename... Args>
  Cache(Args &&...args)
    noexcept(std::is_nothrow_constructible<Persistence, Args...>::value && noexcept(persistence_.Sort(banned_, suspended_)))
    : persistence_(std::forward<Args>(args)...)
  {
    persistence_.Sort(banned_, suspended_);
  }
  Cache(Cache &&)
    noexcept(type_util::AnyV<std::is_nothrow_move_constructible, Persistence, Banned, Suspended>)
    = default;


  Cache &operator=(Cache &&)
    noexcept(type_util::AnyV<std::is_nothrow_move_assignable, Persistence, Banned, Suspended>)
    = default;


  ~Cache()
  {
    try
    {
      persistence_.Flush();
    }
    catch(...)
    {
      try
      {
        FETCH_LOG_WARN("Blackset is not flushed");
      }
      catch(...)
      {}
    }
  }


  template <typename... Args>
  void Blacklist(Args &&... args)
  {
    persistence_.Blacklist(args...);
    banned_.insert(std::forward<Args>(args)...);
  }


  void Quarantine(Timepoint until, ValueType t)
  {
    persistence_.Quarantine(until, t);
    suspended_.emplace(std::move(t), std::move(until));
  }
  template<typename Begin, typename End>
  void Quarantine(Timepoint until, Begin begin, End end)
  {
    while(begin != end) {
      persistence_.Quarantine(until, static_cast<std::add_lvalue_reference<decltype(*begin)>::type>(*begin));
      suspended_.emplace(*begin, until);
      ++begin;
    }
  }


  bool IsBlacklisted(ValueType const &t) const
  {
    return banned_.find(t) != banned_.end() || detail_::Quarantined(suspended_, t);
  }
  auto GetBlacklisted() const
  {
    return banned_;
  }
  auto GetQuarantined() const
  {
    return suspended_;
  }


  void Whitelist(ValueType const &t)
  {
    persistence_.Whitelist(t);
    banned_.erase(t) || suspended_.erase(t);
  }
  void Whitelist(std::initializer_list<ValueType> ts)
  {
    Whitelist(ts.begin(), ts.end());
  }
  template<typename Begin, typename End>
  void Whitelist(Begin begin, End end)
  {
    while(begin != end) {
      persistence_.Whitelist(*begin);
      banned_.erase(*begin) || suspended_.erase(*begin);
      ++begin;
    }
  }

private:
  Persistence persistence_;
  Banned banned_;
  mutable Suspended suspended_;
};


template<class T> class NoPersistence
{
public:
  using Value     = T;
  using Clock     = detail_::Clock;
  using Timepoint = detail_::Timepoint;
  using Banned    = std::unordered_set<T>;
  using Suspended = std::unordered_map<T, Clock::time_point>;

  template<typename... Args>
  NoPersistence(Args &&...args) noexcept(std::is_nothrow_constructible<Banned, Args...>)
    : banned_(std::forward<Args>(args)...)
  {}

  void Sort(Banned &banned, Suspended &) noexcept(noexcept(banned.swap(banned_)))
  {
    banned.swap(banned_);
  }

  static void Flush() noexcept {}

  template<typename... Args> static void Blacklist(Args &&...) noexcept {}

  template<typename... Args> static void Quarantine(Args &&...) noexcept {}

  template<typename... Args> static void Whitelist(Args &&...) noexcept {}
private:
  Banned banned_;
};


template<class T> class Persistence
{
  using Implementation = fetch::storage::KeyValueIndex;

  using StoredType = uint64_t;
  static constexpr auto forever = std::numeric_limits<StoredType>::max();
  // TODO: Implementation::Delete() currently not implemented,
  // this value signifies a whitelisted entry
  static constexpr auto never = std::numeric_limits<StoredType>::min();
public:
  using Value     = T;
  using Clock     = detail_::Clock;
  using Timepoint = detail_::Timepoint;
  using Banned    = std::unordered_set<T>;
  using Suspended = std::unordered_map<T, Clock::time_point>;

  Persistence() = default;
  template<typename... Args> Persistence(Args &&...args)
    : file_(std::forward<Args>(args)...)
  {}

  void Sort(Banned &banned, Suspended &suspended)
  {
    for(auto const &kv: file_)
    {
      switch(kv.second) {
	      case never: continue;
	      case forever: banned.insert(kv.first);
			    break;
	      default: suspended.emplace(kv.first, kv.second);
      }
    }
  }

private:
  Implementation file_;
};


}
}  // namespace generics
}  // namespace fetch






