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

#include "core/mutex.hpp"

#include <mutex>
#include <type_traits>
#include <utility>

namespace fetch {

template <typename T, typename M = Mutex>
class Protected
{
protected:
  mutable M mutex_;
  T         payload_;

  template <class U>
  class Ref
  {
  protected:
    using Type      = U;
    using ConstType = std::add_const_t<U>;

    M &mutex_;
    U &payload_;

    constexpr Ref(M &mutex, U &payload)
      : mutex_(mutex)
      , payload_(payload)
    {
      mutex_.lock();
    }

    friend class Protected<T, M>;

  public:
    ~Ref()
    {
      mutex_.unlock();
    }

    constexpr Type &operator*() noexcept
    {
      return payload_;
    }
    constexpr ConstType &operator*() const noexcept
    {
      return payload_;
    }

    constexpr Type *operator->() noexcept
    {
      return &payload_;
    }
    constexpr ConstType *operator->() const noexcept
    {
      return &payload_;
    }
  };

  using reference       = Ref<T>;
  using const_reference = Ref<std::add_const_t<T>>;

public:
  template <typename... Args>
  explicit Protected(Args &&... args);
  ~Protected()                 = default;
  Protected(Protected const &) = delete;
  Protected(Protected &&)      = delete;

  Protected &operator=(Protected const &) = delete;
  Protected &operator=(Protected &&) = delete;

  constexpr reference LockedRef()
  {
    return {mutex_, payload_};
  }
  constexpr const_reference LockedRef() const
  {
    return {mutex_, payload_};
  }
  constexpr const_reference LockedCRef() const
  {
    return {mutex_, payload_};
  }

  template <typename Handler>
  void ApplyVoid(Handler &&handler)
  {
    static_assert(std::is_void<decltype(handler(payload_))>::value,
                  "Use this method with void handlers only");

    FETCH_LOCK(mutex_);

    handler(payload_);
  }

  template <typename Handler>
  void ApplyVoid(Handler &&handler) const
  {
    static_assert(std::is_void<decltype(handler(payload_))>::value,
                  "Use this method with void handlers only");

    FETCH_LOCK(mutex_);

    handler(payload_);
  }

  template <typename Handler>
  auto Apply(Handler &&handler) -> decltype(handler(payload_))
  {
    static_assert(!std::is_void<decltype(handler(payload_))>::value,
                  "Use this method with non-void handlers only");

    FETCH_LOCK(mutex_);

    return handler(payload_);
  }

  template <typename Handler>
  auto Apply(Handler &&handler) const -> decltype(handler(payload_))
  {
    static_assert(!std::is_void<decltype(handler(payload_))>::value,
                  "Use this method with non-void handlers only");

    FETCH_LOCK(mutex_);

    return handler(payload_);
  }
};

template <typename T, typename M>
template <typename... Args>
Protected<T, M>::Protected(Args &&... args)
  : payload_(std::forward<Args>(args)...)
{}

}  // namespace fetch
