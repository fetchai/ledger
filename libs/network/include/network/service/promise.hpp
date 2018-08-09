#pragma once

#include "core/byte_array/byte_array.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "core/serializers/exception.hpp"
#include "network/service/types.hpp"

#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>

namespace fetch {
namespace service {
namespace details {
class PromiseImplementation
{
public:
  using promise_counter_type = uint64_t;
  using byte_array_type      = byte_array::ConstByteArray;
  typedef std::function<void (void)> callback_type;
  typedef enum
    {
      NONE, SUCCESS, FAIL
    }
    Conclusion;

  PromiseImplementation()
  {
    LOG_STACK_TRACE_POINT;

    id_                = next_promise_id();
  }

  void ConcludeSuccess(void)
  {
    if (conclusion_.exchange(SUCCESS) == NONE)
    {
      auto func = on_success_;
      if (func)
      {
        func();
      }
    }
  }

  void ConcludeFail(void)
  {
    if (conclusion_.exchange(FAIL) == NONE)
    {
      auto func = on_fail_;
      if (func)
      {
        func();
      }
    }
  }

  void Fulfill(byte_array_type const &value)
  {
    LOG_STACK_TRACE_POINT;

    value_     = value;
    fulfilled_ = true;
    ConcludeSuccess();
  }

  PromiseImplementation &Then(callback_type func)
  {
    on_success_ = func;
    if (fulfilled_)
    {
      if (!failed_)
      {
        ConcludeSuccess(); // if this is called >1, it will protect itself.
      }
    }
    return *this;
  }
  PromiseImplementation &Else(callback_type func)
  {
    on_fail_ = func;
    if (fulfilled_)
    {
      if (failed_ || connection_closed_)
      {
        ConcludeFail(); // if this is called >1, it will protect itself.
      }
    }
    return *this;
  }

  void Fail(serializers::SerializableException const &excp)
  {
    LOG_STACK_TRACE_POINT;

    exception_ = excp;
    failed_    = true;  // Note that order matters here due to threading!
    fulfilled_ = true;
    ConcludeFail();
  }

  void ConnectionFailed()
  {
    LOG_STACK_TRACE_POINT;

    connection_closed_ = true;
    fulfilled_         = true; // Note that order matters here due to threading!
    ConcludeFail();
  }

  serializers::SerializableException exception() const { return exception_; }
  bool                               is_fulfilled() const { return fulfilled_; }
  bool                               has_failed() const { return failed_; }
  bool                               is_connection_closed() const { return connection_closed_; }

  byte_array_type value() const { return value_; }
  uint64_t        id() const { return id_; }

private:
  serializers::SerializableException exception_;
  std::atomic<bool>                  connection_closed_{false};
  std::atomic<bool>                  fulfilled_{false};
  std::atomic<bool>                  failed_{false};
  std::atomic<uint64_t>              id_;
  byte_array_type                    value_;
  std::atomic<Conclusion>            conclusion_{NONE};
  callback_type on_success_;
  callback_type on_fail_;

  static uint64_t next_promise_id()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(counter_mutex_);
    uint64_t                             promise = promise_counter_;
    ++promise_counter_;
    return promise;
  }

  static promise_counter_type promise_counter_;
  static fetch::mutex::Mutex  counter_mutex_;
};

}  // namespace details

class Promise
{
public:
  using promise_type         = typename details::PromiseImplementation;
  using promise_counter_type = typename promise_type::promise_counter_type;
  using shared_promise_type  = std::shared_ptr<promise_type>;

  using callback_type = details::PromiseImplementation::callback_type;

  Promise()
  {
    LOG_STACK_TRACE_POINT;

    reference_ = std::make_shared<promise_type>();
    created_   = std::chrono::system_clock::now();
  }

  bool Wait(bool const &throw_exception)
  {
    return Wait(std::numeric_limits<double>::infinity(), throw_exception);
  }

  typedef enum
    {
      OK = 0,
      FAILED = 1,
      CLOSED = 2,
      WAITING = 4,
    }
  Status;

  Status GetStatus()
  {
    return Status(
                  ( has_failed() ? 1 : 0 ) +
                  ( is_connection_closed() ? 2 : 0 ) +
                  ( !is_fulfilled() ? 4 : 0 )
                  );
  }

  static std::string DescribeStatus(Status s)
  {
    const char *states[8] = {
      "OK.",
      "Failed.",
      "Closed.",
      "Failed.",
      "Timeout.",
      "Failed.",
      "Closed.",
      "Failed."
    };
    return states[ int(s) & 0x07 ];
  }

  Status WaitLoop(int milliseconds, int cycles)
  {
    auto s = GetStatus();
    while(cycles>0 && s == WAITING)
    {
      cycles-=1;
      s = GetStatus();
      if (s != WAITING) return s;

      FETCH_LOG_PROMISE();
      Wait(milliseconds, false);
      s = GetStatus();
      if (s != WAITING) return s;
    }
    return WAITING;
  }

  bool Wait(int const &time) { return Wait(double(time)); }

  bool Wait(double const timeout         = std::numeric_limits<double>::infinity(),
            bool const & throw_exception = true)
  {
    LOG_STACK_TRACE_POINT;

    while (!is_fulfilled())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
      double                                ms =
          double(std::chrono::duration_cast<std::chrono::milliseconds>(end - created_).count());
      if ((ms > timeout) && (!is_fulfilled()))
      {
        fetch::logger.Warn("Connection timed out! ", ms, " vs. ", timeout);
        return false;
      }
    }

    if (is_connection_closed())
    {
      return false;
    }

    if (has_failed())
    {
      fetch::logger.Warn("Connection failed!");

      if (throw_exception)
        throw reference_->exception();
      else
        return false;
    }
    return true;
  }

  template <typename T>
  T As()
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_PROMISE();
    if (!Wait())
    {
      TODO_FAIL("Timeout or connection lost");
    }

    serializer_type ser(reference_->value());
    T               ret;
    ser >> ret;
    return ret;
  }

  template <typename T>
  void As(T &ret)
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_PROMISE();
    if (!Wait())
    {
      TODO_FAIL("Timeout or connection lost");
    }

    serializer_type ser(reference_->value());
    ser >> ret;
  }

  template <typename T>
  void As(T &ret) const
  {
    LOG_STACK_TRACE_POINT;
    if (!is_fulfilled())
    {
      TODO_FAIL("Don't call non-waity As until promise filled");
    }

    serializer_type ser(reference_->value());
    ser >> ret;
  }

  template <typename T>
  operator T()
  {
    LOG_STACK_TRACE_POINT;
    return As<T>();
  }

  bool is_fulfilled() const { return reference_->is_fulfilled(); }
  bool has_failed() const { return reference_->has_failed(); }
  bool is_connection_closed() const { return reference_->is_connection_closed(); }
  Promise &Then(callback_type func) { reference_ -> Then(func); return *this; }
  Promise &Else(callback_type func) { reference_ -> Else(func); return *this; }

  shared_promise_type  reference() { return reference_; }
  promise_counter_type id() const { return reference_->id(); }

private:
  shared_promise_type                   reference_;
  std::chrono::system_clock::time_point created_;
};
}  // namespace service
}  // namespace fetch
