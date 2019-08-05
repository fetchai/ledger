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

#include "core/threading/protect.hpp"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>

namespace fetch {

template <typename T, typename M = std::mutex>
class SynchronisedState
{
private:
  mutable std::condition_variable condition_{};
  Protect<T, M>                   protected_payload_;

public:
  template <typename... Args>
  explicit SynchronisedState(Args &&... args);
  SynchronisedState(SynchronisedState const &) = delete;
  SynchronisedState(SynchronisedState &&)      = delete;
  ~SynchronisedState()                         = default;

  SynchronisedState &operator=(SynchronisedState const &) = delete;
  SynchronisedState &operator=(SynchronisedState &&) = delete;

  template <typename Handler>
  auto WithLock(Handler &&handler)
      -> std::enable_if_t<std::is_void<decltype(protected_payload_.WithLock(handler))>::value, void>
  {
    protected_payload_.WithLock(handler);
    condition_.notify_all();
  }

  template <typename Handler>
  auto WithLock(Handler &&handler) const
      -> std::enable_if_t<std::is_void<decltype(protected_payload_.WithLock(handler))>::value, void>
  {
    protected_payload_.WithLock(handler);
    condition_.notify_all();
  }

  template <typename Handler>
  auto WithLock(Handler &&handler)
      -> std::enable_if_t<!std::is_void<decltype(protected_payload_.WithLock(handler))>::value,
                          decltype(protected_payload_.WithLock(handler))>
  {
    auto const result = protected_payload_.WithLock(handler);
    condition_.notify_all();

    return std::move(result);
  }

  template <typename Handler>
  auto WithLock(Handler &&handler) const
      -> std::enable_if_t<!std::is_void<decltype(protected_payload_.WithLock(handler))>::value,
                          decltype(protected_payload_.WithLock(handler)) const>
  {
    auto const result = protected_payload_.WithLock(handler);
    condition_.notify_all();

    return std::move(result);
  }

  template <typename Predicate>
  void Wait(Predicate &&predicate) const;
  template <typename Predicate, typename R, typename P>
  bool Wait(Predicate &&predicate, std::chrono::duration<R, P> const &max_wait_time) const;
};

template <typename T, typename M>
template <typename... Args>
SynchronisedState<T, M>::SynchronisedState(Args &&... args)
  : protected_payload_{std::forward<Args>(args)...}
{}

template <typename T, typename M>
template <typename Predicate>
void SynchronisedState<T, M>::Wait(Predicate &&predicate) const
{
  std::unique_lock<M> lock{protected_payload_.mutex_};

  condition_.wait(lock,
                  [this, predicate]() -> bool { return predicate(protected_payload_.payload_); });
}

template <typename T, typename M>
template <typename Predicate, typename R, typename P>
bool SynchronisedState<T, M>::Wait(Predicate &&                       predicate,
                                   std::chrono::duration<R, P> const &max_wait_time) const
{
  std::unique_lock<M> lock{protected_payload_.mutex_};

  return condition_.wait_for(lock, max_wait_time, [this, predicate]() -> bool {
    return predicate(protected_payload_.payload_);
  });
}

}  // namespace fetch
