#pragma once

#include "network/service/promise.hpp"

namespace fetch {

namespace network {

template <class TYPE>
class PromiseOf
{
public:
  typedef fetch::service::Promise promise_type;

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

