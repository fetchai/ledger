#ifndef MILLI_TIMER_HPP
#define MILLI_TIMER_HPP

#include <iostream>
#include <string>

#include <chrono>
#include "core/logger.hpp"

namespace fetch
{
namespace generics
{

class MilliTimer
{
public:
  MilliTimer(const MilliTimer &rhs)            = delete;
  MilliTimer(MilliTimer &&rhs)                 = delete;
  MilliTimer &operator=(const MilliTimer &rhs) = delete;
  MilliTimer &operator=(MilliTimer &&rhs)      = delete;
  bool operator==(const MilliTimer &rhs) const = delete;
  bool operator<(const MilliTimer &rhs) const  = delete;

  explicit MilliTimer(const char *name, int64_t threshold = 100)
  {
     start_ = std::chrono::steady_clock::now();
     name_ = name;
     threshold_ = threshold;
  }

  virtual ~MilliTimer()
  {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
      (std::chrono::steady_clock::now() - start_);
    if (duration.count() > threshold_)
    {
      fetch::logger.Warn("Too many milliseconds: ", duration.count(), " at ", name_);
    }
    else
    {
      fetch::logger.Debug("Consumed milliseconds: ", duration.count(), " at ", name_);
    }
  }
private:
// members here.

  std::chrono::steady_clock::time_point start_;
  std::string name_;
  int64_t threshold_;
};

}
}

#endif //MILLI_TIMER_HPP
