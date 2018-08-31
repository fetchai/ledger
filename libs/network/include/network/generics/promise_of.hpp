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

namespace fetch {
namespace network {

/**
 * Simple wrapper around the service promise which mandates the return time from the underlying
 * promise.
 *
 * @tparam TYPE The expected return type of the promise
 */
template <class TYPE>
class PromiseOf
{
public:
  using Promise = fetch::service::Promise;
  using PromiseBuilder = fetch::service::details::PromiseBuilder;

  // Construction / Destruction
  explicit PromiseOf(Promise promise);
  PromiseOf(PromiseOf const &rhs) = default;
  ~PromiseOf() = default;

  // Operators
  PromiseOf &operator=(PromiseOf const &rhs) = default;
  PromiseOf &operator=(PromiseOf &&rhs) noexcept = default;
  explicit operator bool() const;

  // Promise Accessors
  TYPE Get() const;
  bool Wait(uint32_t timeout_ms = std::numeric_limits<uint32_t>::max(),
            bool throw_exception = true) const;

  Promise const &GetInnerPromise() const { return promise_; }
  PromiseBuilder WithHandlers() { return promise_->WithHandlers(); }

private:
  Promise promise_;
};

/**
 * Construct a PromiseOf from and specified Promise
 *
 * @tparam TYPE The expected return type of the promise
 * @param promise The promise value
 */
template <typename TYPE>
inline PromiseOf<TYPE>::PromiseOf(Promise promise)
  : promise_(std::move(promise))
{
}

/**
 * Gets the value of the promise
 *
 * @tparam TYPE The expected return type of the promise
 * @return Return the value from the promise, or throw an exception if not possible
 */
template <typename TYPE>
inline TYPE PromiseOf<TYPE>::Get() const
{
  return promise_->As<TYPE>();
}

/**
 * Checks to see if the promise has been fulfilled
 *
 * @tparam TYPE The expected return type of the promise
 * @return true if the promise has been fulfilled, otherwise false
 */
template <typename TYPE>
inline PromiseOf<TYPE>::operator bool() const
{
  return promise_->IsSuccessful();
}

/**
 * Waits for the promise to complete
 *
 * @tparam TYPE The expected return type of the promise
 * @param timeout_ms The timeout in milliseconds to wait for this promise to conclude
 * @param throw_exception Signal if the function should throw an exception in the case of an error
 * @return true if the promise has been fulfilled (given the constraints), otherwise false
 */
template <typename TYPE>
inline bool PromiseOf<TYPE>::Wait(uint32_t timeout_ms, bool throw_exception) const
{
  FETCH_LOG_PROMISE();
  promise_->Wait(timeout_ms, throw_exception);
  return promise_->IsSuccessful();
}

}  // namespace network
}  // namespace fetch
