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

#include <mutex>
#include <type_traits>
#include <utility>

namespace fetch {

template <typename T, typename M = std::mutex>
class Protect
{
private:
  mutable M mutex_;
  T         payload_;

public:
  template <typename... Args>
  explicit Protect(Args &&...);
  ~Protect()               = default;
  Protect(Protect const &) = delete;
  Protect(Protect &&)      = delete;

  Protect &operator=(Protect const &) = delete;
  Protect &operator=(Protect &&) = delete;

  template <typename Handler>
  auto WithLock(Handler &&handler)
      -> std::enable_if_t<std::is_void<decltype(handler(payload_))>::value, void>
  {
    std::lock_guard<M> lock(mutex_);

    handler(payload_);
  }

  template <typename Handler>
  auto WithLock(Handler &&handler) const
      -> std::enable_if_t<std::is_void<decltype(handler(payload_))>::value, void>
  {
    std::lock_guard<M> lock(mutex_);

    handler(payload_);
  }

  template <typename Handler>
  auto WithLock(Handler &&handler)
      -> std::enable_if_t<!std::is_void<decltype(handler(payload_))>::value,
                          decltype(handler(payload_))>
  {
    std::lock_guard<M> lock(mutex_);

    return handler(payload_);
  }

  template <typename Handler>
  auto WithLock(Handler &&handler) const
      -> std::enable_if_t<!std::is_void<decltype(handler(payload_))>::value,
                          decltype(handler(payload_)) const>
  {
    std::lock_guard<M> lock(mutex_);

    return handler(payload_);
  }

  template <typename TT, typename MM>
  friend class SynchronisedState;
};

template <typename T, typename M>
template <typename... Args>
Protect<T, M>::Protect(Args &&... args)
  : mutex_{}
  , payload_{std::forward<Args>(args)...}
{}

}  // namespace fetch
