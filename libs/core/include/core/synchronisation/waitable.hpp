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

#include "core/synchronisation/protected.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>

namespace fetch {

template <typename T, typename M = std::mutex>
class Waitable : Protected<T, M>
{
  using ProtectedPayload = Protected<T, M>;
  using ProtectedPayload::mutex_;
  using ProtectedPayload::payload_;

  mutable std::condition_variable condition_{};

  template <class U>
  class Ref : public ProtectedPayload::template Ref<U>
  {
    using Parent    = typename ProtectedPayload::template Ref<U>;
    using Type      = U;
    using ConstType = std::add_const_t<U>;

    std::condition_variable &condition_;

    constexpr Ref(std::condition_variable &condition, M &mutex, U &payload)
      : Parent(mutex, payload)
      , condition_(condition)
    {}

    friend class Waitable<T, M>;

  public:
    ~Ref()
    {
      condition_.notify_all();
    }
  };

  using reference       = Ref<T>;
  using const_reference = Ref<std::add_const_t<T>>;

public:
  template <typename... Args>
  explicit Waitable(Args &&... args);
  Waitable(Waitable const &) = delete;
  Waitable(Waitable &&)      = delete;
  ~Waitable()                = default;

  Waitable &operator=(Waitable const &) = delete;
  Waitable &operator=(Waitable &&) = delete;

  constexpr reference LockedRef()
  {
    return {condition_, mutex_, payload_};
  }
  constexpr const_reference LockedRef() const
  {
    return {condition_, mutex_, payload_};
  }
  constexpr const_reference LockedCRef() const
  {
    return {condition_, mutex_, payload_};
  }

  template <typename Handler>
  void ApplyVoid(Handler &&handler)
  {
    auto payload{ProtectedPayload::LockedRef()};
    handler(*payload);
    condition_.notify_all();
  }

  template <typename Handler>
  void ApplyVoid(Handler &&handler) const
  {
    auto payload{ProtectedPayload::LockedRef()};
    handler(*payload);
    condition_.notify_all();
  }

  template <typename Handler>
  auto Apply(Handler &&handler)
  {
    auto payload{ProtectedPayload::LockedRef()};
    auto result = handler(*payload);
    condition_.notify_all();

    return result;
  }

  template <typename Handler>
  auto Apply(Handler &&handler) const
  {
    auto payload{ProtectedPayload::LockedRef()};
    auto result = handler(*payload);
    condition_.notify_all();

    return result;
  }

  template <typename Predicate>
  void Wait(Predicate &&predicate) const;
  template <typename Predicate, typename R, typename P>
  bool Wait(Predicate &&predicate, std::chrono::duration<R, P> const &max_wait_time) const;
};

template <typename T, typename M>
template <typename... Args>
Waitable<T, M>::Waitable(Args &&... args)
  : ProtectedPayload(std::forward<Args>(args)...)
{}

template <typename T, typename M>
template <typename Predicate>
void Waitable<T, M>::Wait(Predicate &&predicate) const
{
  std::unique_lock<M> lock{mutex_};

  condition_.wait(lock, [this, predicate]() -> bool { return predicate(payload_); });
}

template <typename T, typename M>
template <typename Predicate, typename R, typename P>
bool Waitable<T, M>::Wait(Predicate &&                       predicate,
                          std::chrono::duration<R, P> const &max_wait_time) const
{
  std::unique_lock<M> lock{mutex_};

  return condition_.wait_for(lock, max_wait_time,
                             [this, predicate]() -> bool { return predicate(payload_); });
}

}  // namespace fetch
