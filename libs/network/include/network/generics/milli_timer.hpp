#ifndef MILLI_TIMER_HPP
#define MILLI_TIMER_HPP

#include <iostream>
#include <string>

#include "core/logger.hpp"
#include <chrono>

namespace fetch {
namespace generics {

class MilliTimer
{
public:

  static constexpr char const *LOGGING_NAME = "MilliTimer";

  MilliTimer(const MilliTimer &rhs) = delete;
  MilliTimer(MilliTimer &&rhs)      = delete;
  MilliTimer &operator=(const MilliTimer &rhs) = delete;
  MilliTimer &operator=(MilliTimer &&rhs)             = delete;
  bool        operator==(const MilliTimer &rhs) const = delete;
  bool        operator<(const MilliTimer &rhs) const  = delete;

  explicit MilliTimer(const char *name, int64_t threshold = 100)
  {
    start_     = std::chrono::steady_clock::now();
    name_      = name;
    threshold_ = threshold;

    if (!threshold)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,"Starting millitimer for ", name_);
    }
  }

  virtual ~MilliTimer()
  {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_);
    if (duration.count() > threshold_)
    {
      FETCH_LOG_WARN(LOGGING_NAME,"Too many milliseconds: ", duration.count(), " at ", name_);
    }
    else
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,"Consumed milliseconds: ", duration.count(), " at ", name_);
    }
  }

private:
  // members here.

  std::chrono::steady_clock::time_point start_;
  std::string                           name_;
  int64_t                               threshold_;
};

}  // namespace generics
}  // namespace fetch

#endif  // MILLI_TIMER_HPP
