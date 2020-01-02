#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "network/generics/resolvable.hpp"
#include "network/service/promise.hpp"

namespace fetch {
namespace network {

/**
 * Simple wrapper around the service promise which mandates the return time from the underlying
 * promise.
 *
 * @tparam RESULT The expected return type of the promise
 */
template <typename RESULT>
class PromiseOf : public ResolvableTo<RESULT>
{
public:
  using Promise        = service::Promise;
  using State          = fetch::service::PromiseState;
  using PromiseBuilder = fetch::service::details::PromiseBuilder;
  using PromiseCounter = fetch::service::PromiseCounter;

  // Construction / Destruction
  PromiseOf() = default;
  explicit PromiseOf(Promise promise);
  PromiseOf(PromiseOf const &rhs)  = default;
  PromiseOf(PromiseOf &&) noexcept = default;
  ~PromiseOf() override            = default;

  // Promise Accessors
  bool Wait(bool throw_exception = true) const;

  bool GetResult(RESULT &result) const
  {
    return promise_->GetResult(result);
  }

  Promise const &GetInnerPromise() const
  {
    return promise_;
  }

  PromiseBuilder WithHandlers()
  {
    return promise_->WithHandlers();
  }

  bool empty() const
  {
    return !promise_;
  }

  State GetState() override
  {
    return promise_->state();
  }

  PromiseCounter id() const override
  {
    return promise_->id();
  }

  std::string const &name() const
  {
    return promise_->name();
  }

  // Operators
  explicit operator bool() const;

  PromiseOf &operator=(PromiseOf const &rhs) = default;
  PromiseOf &operator=(PromiseOf &&rhs) noexcept = default;

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
PromiseOf<TYPE>::PromiseOf(Promise promise)
  : promise_(std::move(promise))
{}

/**
 * Checks to see if the promise has been fulfilled
 *
 * @tparam TYPE The expected return type of the promise
 * @return true if the promise has been fulfilled, otherwise false
 */
template <typename TYPE>
PromiseOf<TYPE>::operator bool() const
{
  return promise_ && promise_->IsSuccessful();
}

/**
 * Waits for the promise to complete
 *
 * @tparam TYPE The expected return type of the promise
 * @param throw_exception Signal if the function should throw an exception in the case of an error
 * @return true if the promise has been fulfilled (given the constraints), otherwise false
 */
template <typename TYPE>
bool PromiseOf<TYPE>::Wait(bool throw_exception) const
{
  promise_->Wait(throw_exception);
  return promise_->IsSuccessful();
}

}  // namespace network
}  // namespace fetch
