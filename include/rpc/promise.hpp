#ifndef RPC_PROMISE_HPP
#define RPC_PROMISE_HPP

#include "serializer/exception.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace fetch {
namespace rpc {
namespace details {
class PromiseImplementation {
 public:
  typedef uint64_t promise_counter_type;
  typedef byte_array::ReferencedByteArray byte_array_type;

  PromiseImplementation() {
    fulfilled_ = false;
    failed_ = false;
    id_ = next_promise_id();
  }

  void Fulfill(byte_array_type const &value) {
    value_ = value;
    fulfilled_ = true;
  }

  void Fail(serializers::SerializableException const &excp) {
    exception_ = excp;
    failed_ = true;  // Note that order matters here due to threading!
    fulfilled_ = true;
  }

  serializers::SerializableException exception() const { return exception_; }
  bool is_fulfilled() const { return fulfilled_; }
  bool has_failed() const { return failed_; }

  byte_array_type value() const { return value_; }
  uint64_t id() const { return id_; }

 private:
  serializers::SerializableException exception_;
  std::atomic<bool> fulfilled_;
  std::atomic<bool> failed_;
  std::atomic<uint64_t> id_;
  byte_array_type value_;

  static uint64_t next_promise_id() {
    std::lock_guard<std::mutex> lock(counter_mutex_);
    uint64_t promise = promise_counter_;
    ++promise_counter_;
    return promise;
  }

  static promise_counter_type promise_counter_;
  static std::mutex counter_mutex_;
};
PromiseImplementation::promise_counter_type
    PromiseImplementation::promise_counter_ = 0;
std::mutex PromiseImplementation::counter_mutex_;
}

class Promise {
 public:
  typedef typename details::PromiseImplementation promise_type;
  typedef typename promise_type::promise_counter_type promise_counter_type;
  typedef std::shared_ptr<promise_type> shared_promise_type;

  Promise() { reference_ = std::make_shared<promise_type>(); }

  void Wait() {
    while (!is_fulfilled()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if (has_failed()) {
      throw reference_->exception();
    }
  }

  template <typename T>
  T As() {
    Wait();
    serializer_type ser(reference_->value());
    T ret;
    ser >> ret;
    return ret;
  }

  template <typename T>
  operator T() {
    return As<T>();
  }

  bool is_fulfilled() const { return reference_->is_fulfilled(); }
  bool has_failed() const { return reference_->has_failed(); }

  shared_promise_type reference() { return reference_; }
  promise_counter_type id() const { return reference_->id(); }

 private:
  shared_promise_type reference_;
};
};
};

#endif
