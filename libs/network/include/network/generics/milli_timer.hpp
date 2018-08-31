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

  using Clock = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;

  static constexpr char const *LOGGING_NAME = "MilliTimer";

  MilliTimer(MilliTimer const &rhs) = delete;
  MilliTimer(MilliTimer &&rhs)      = delete;
  MilliTimer &operator=(MilliTimer const &rhs) = delete;
  MilliTimer &operator=(MilliTimer &&rhs)             = delete;

  explicit MilliTimer(std::string name, int64_t threshold = 100)
    : start_(Clock::now())
    , name_(std::move(name))
    , threshold_(threshold)
  {
    if (!threshold)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,"Starting millitimer for ", name_);
    }
  }

  virtual ~MilliTimer()
  {
    auto const duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - start_
    );

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

  Timepoint   start_;
  std::string name_;
  int64_t     threshold_;
};

}  // namespace generics
}  // namespace fetch

#endif  // MILLI_TIMER_HPP
