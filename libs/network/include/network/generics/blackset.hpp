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

#include <initializer_list>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <chrono>
#include <iostream>

#include <core/mutex.hpp>

namespace fetch {
namespace generics {

namespace detail_ {
  using Clock = std::chrono::system_clock;

  template<class Set, class Key>
  inline typename Set::const_iterator FindQuarantined(Set &suspended, Key const &key)
  {
    auto where{suspended.find(key)};
    if(where != suspended.cend()) {
  	  if(where->second > Clock::now()) return where;
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

template <class T, class Mx = mutex::Mutex>
class Blackset
{
public:
  using mutex_type = Mx;
  using ValueType  = T;
  using Clock = detail_::Clock;
  using Timepoint  = Clock::time_point;

private:
  using Banned  = std::unordered_set<T>;
  using Suspended = std::unordered_map<T, Clock::time_point>;

public:
  Blackset(mutex_type &mutex)
    : mutex_(&mutex)
  {}
  Blackset(Blackset const &) = default;
  Blackset(Blackset &&)      = default;
  template <typename... Args>
  Blackset(Mx &mutex, Args &&... args)
    : mutex_(&mutex)
    , banned_(std::forward<Args>(args)...)
  {}


  Blackset &operator=(Blackset const &) = default;
  Blackset &operator=(Blackset &&) noexcept(std::is_nothrow_move_assignable<Banned>::value) = default;
  template <typename U>
  Blackset &operator=(U &&u) noexcept(std::is_nothrow_assignable<Banned, U>::value)
  {
    banned_ = std::forward<U>(u);
    return *this;
  }


  template <typename... Args>
  Blackset &Blacklist(Args &&... args)
  {
    FETCH_LOCK(*mutex_);
    banned_.insert(std::forward<Args>(args)...);
    return *this;
  }


  Blackset &Quarantine(Timepoint until, ValueType t)
  {
    FETCH_LOCK(*mutex_);
    suspended_.emplace(std::move(t), std::move(until));
    return *this;
  }
  template<typename Begin, typename End>
  Blackset &Quarantine(Timepoint const &until, Begin begin, End end)
  {
    FETCH_LOCK(*mutex_);
    while(begin != end) {
	    suspended_.emplace(*begin, until);
	    ++begin;
    }
    return *this;
  }


  bool IsBlacklisted(ValueType const &t) const
  {
    FETCH_LOCK(*mutex_);
    return Blacklisted(t);
  }
  Banned GetBlacklisted() const
  {
    FETCH_LOCK(*mutex_);
    return banned_;
  }
  Suspended GetQuarantined() const
  {
	  FETCH_LOCK(*mutex_);
	  return suspended_;
  }


  Blackset &Whitelist(ValueType const &t)
  {
    FETCH_LOCK(*mutex_);
    banned_.erase(t) || suspended_.erase(t);
    return *this;
  }
  Blackset &Whitelist(std::initializer_list<ValueType> ts)
  {
    return Whitelist(ts.begin(), ts.end());
  }
  template<typename Begin, typename End>
  Blackset &Whitelist(Begin begin, End end)
  {
    FETCH_LOCK(*mutex_);
    while(begin != end) {
	    banned_.erase(*begin) || suspended_.erase(*begin);
	    ++begin;
    }
    return *this;
  }

protected:
  bool Blacklisted(ValueType const &t) const
  {
    return banned_.find(t) != banned_.end() || Quarantined(t);
  }
private:
  using ConstIterator = typename Suspended::const_iterator;

  bool Quarantined(ValueType const &t) const
  {
	  return detail_::Quarantined(suspended_, t);
  }

  Mx * mutex_;
  Banned banned_;
  mutable Suspended suspended_;
};

// Is there a better way to do it than to retype the whole thing?
// Explicit instantiation is still too much, as every public method needs to be reiterated.

template <typename T>
class Blackset<T, void>
{
public:
  using mutex_type = void;
  using ValueType  = T;
  using Clock = detail_::Clock;
  using Timepoint  = Clock::time_point;

private:
  using Banned  = std::unordered_set<T>;
  using Suspended = std::unordered_map<T, Clock::time_point>;

public:
  Blackset() = default;
  Blackset(Blackset const &) = default;
  Blackset(Blackset &&)      = default;
  template <typename... Args>
  Blackset(Args &&... args)
    : banned_(std::forward<Args>(args)...)
  {}


  Blackset &operator=(Blackset const &) = default;
  Blackset &operator=(Blackset &&) noexcept(std::is_nothrow_move_assignable<Banned>::value) = default;
  template <typename U>
  Blackset &operator=(U &&u) noexcept(std::is_nothrow_assignable<Banned, U>::value)
  {
    banned_ = std::forward<U>(u);
    return *this;
  }


  template <typename... Args>
  Blackset &Blacklist(Args &&... args)
  {
    banned_.insert(std::forward<Args>(args)...);
    return *this;
  }


  Blackset &Quarantine(Timepoint until, ValueType t)
  {
    suspended_.emplace(std::move(t), std::move(until));
    return *this;
  }
  template<typename Begin, typename End>
  Blackset &Quarantine(Timepoint const &until, Begin begin, End end)
  {
    while(begin != end) {
	    suspended_.emplace(*begin, until);
	    ++begin;
    }
    return *this;
  }


  bool IsBlacklisted(ValueType const &t) const
  {
    return Blacklisted(t);
  }
  Banned GetBlacklisted() const
  {
    return banned_;
  }
  Suspended GetQuarantined() const
  {
    return suspended_;
  }


  Blackset &Whitelist(ValueType const &t)
  {
    banned_.erase(t) || suspended_.erase(t);
    return *this;
  }
  Blackset &Whitelist(std::initializer_list<ValueType> ts)
  {
    return Whitelist(ts.begin(), ts.end());
  }
  template<typename Begin, typename End>
  Blackset &Whitelist(Begin begin, End end)
  {
    while(begin != end) {
	    banned_.erase(*begin) || suspended_.erase(*begin);
	    ++begin;
    }
    return *this;
  }

protected:
  bool Blacklisted(ValueType const &t) const
  {
    return banned_.find(t) != banned_.end() || Quarantined(t);
  }

private:
  bool Quarantined(ValueType const &t) const
  {
	  return detail_::Quarantined(suspended_, t);
  }

  Banned banned_;
  mutable Suspended suspended_;
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
  using Suspended1 = std::unordered_map<T1, Clock::time_point>;
  using Suspended2 = std::unordered_map<T2, Clock::time_point>;

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
    if(banned2_.find(t2) != banned2_.end()) {
	    banned1_.insert(t1);
	    return true;
    }
    ConstIterator2 where{detail_::FindQuarantined(suspended2_, t2)};
    if(where != suspended2_.end()) {
	    suspended1_.insert(t1, where->second);
	    return true;
    }
    return false;
  }
private:
  using ConstIterator2 = typename Suspended2::const_iterator;

  Banned1 banned1_;
  Banned2 banned2_;
  mutable Suspended1 suspended1_;
  mutable Suspended2 suspended2_;
};


}  // namespace generics
}  // namespace fetch
