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

template <class TYPE>
class PromiseOf
{
public:
  using promise_type = fetch::service::Promise;

  PromiseOf(promise_type &promise) { this->promise_ = promise; }

  PromiseOf(const PromiseOf &rhs) { promise_ = rhs.promise_; }

  PromiseOf operator=(const PromiseOf &rhs) { promise_ = rhs.promise_; }

  PromiseOf operator=(PromiseOf &&rhs) { promise_ = std::move(rhs.promise_); }

  TYPE Get() { return promise_.As<TYPE>(); }

  TYPE get() { return promise_.As<TYPE>(); }

  operator bool() const { return promise_.is_fulfilled(); }

  bool Wait()
  {
    promise_.Wait();
    return promise_.is_fulfilled();
  }

private:
  promise_type promise_;
};

}  // namespace network
}  // namespace fetch
