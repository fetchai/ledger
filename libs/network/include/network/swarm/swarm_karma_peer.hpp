#pragma once

#include <functional>
#include <time.h>

#include "swarm_peer_location.hpp"

namespace fetch {
namespace swarm {

class SwarmKarmaPeer
{
public:
  friend class SwarmKarmaPeers;
  SwarmKarmaPeer(const SwarmKarmaPeer &rhs)
      : location_(rhs.location_), karma_(rhs.karma_), karmaTime_(rhs.karmaTime_)
  {}

  explicit SwarmKarmaPeer(const SwarmPeerLocation &loc, double karma = 0.0)
      : location_(loc), karma_(karma), karmaTime_(GetCurrentTime())
  {}

  explicit SwarmKarmaPeer(const std::string &loc, double karma = 0.0)
      : location_(loc), karma_(karma), karmaTime_(GetCurrentTime())
  {}

  SwarmKarmaPeer(SwarmKarmaPeer &&rhs)
      : location_(std::move(rhs.location_))
      , karma_(std::move(rhs.karma_))
      , karmaTime_(std::move(rhs.karmaTime_))
  {}

  virtual ~SwarmKarmaPeer() {}

  SwarmKarmaPeer operator=(const SwarmKarmaPeer &rhs)
  {
    location_  = rhs.location_;
    karma_     = rhs.karma_;
    karmaTime_ = rhs.karmaTime_;
    return *this;
  }

  bool operator==(SwarmKarmaPeer &rhs) const
  {
    return location_ == rhs.location_;
  }

  bool operator==(const SwarmPeerLocation &rhs) const
  {
    return location_ == rhs;
  }

  bool operator==(const std::string &host) const { return location_ == host; }

  const SwarmPeerLocation &GetHost() const { return location_; }

  SwarmKarmaPeer operator=(SwarmKarmaPeer &&rhs)
  {
    location_  = std::move(rhs.location_);
    karma_     = std::move(rhs.karma_);
    karmaTime_ = std::move(rhs.karmaTime_);
    return *this;
  }

  void AddKarma(double karmaChange)
  {
    Age();
    karma_ += karmaChange;
  }

  static std::function<time_t()> &getCurrentTimeCBRef()
  {
    static std::function<time_t()> cb;
    return cb;
  }

  static void ToGetCurrentTime(std::function<time_t()> cb)
  {
    getCurrentTimeCBRef() = cb;
  }

  static time_t GetCurrentTime()
  {
    if (getCurrentTimeCBRef())
    {
      return getCurrentTimeCBRef()();
    }
    return 0;
  }

  const SwarmPeerLocation GetLocation(void) const { return location_; }

  static double ComputeKarmaForTime(double karmaValue, time_t timeStart,
                                    time_t timeFinish)
  {
    // TODO(katie) This should probably be some half-life asymptotic function.
    // Might be expensive to compute tho. Consider making the internal store
    // mutable so we can compute an uptodate cache inside the < func?
    if (karmaValue == 0.0)
    {
      return 0;
    }
    if (karmaValue > 0.0)
    {
      double ageings = double(timeFinish - timeStart) / 5.0;
      double k       = (1.0 - ageings) * karmaValue;
      if (k < 0.0) return 0.0;
      return k;
    }
    else
    {
      double ageings = double(timeFinish - timeStart) / 10.0;
      double k       = (1.0 - ageings) * karmaValue;
      if (k > 0.0) return 0.0;
      return k;
    }
  }

  void Age()
  {
    auto now   = GetCurrentTime();
    karma_     = ComputeKarmaForTime(karma_, karmaTime_, now);
    karmaTime_ = now;
  }

  double GetKarma() { return karma_; }

  double GetCurrentKarma() const
  {
    auto now = GetCurrentTime();
    return ComputeKarmaForTime(karma_, karmaTime_, now);
  }

  double GetCurrentKarma()
  {
    Age();
    return karma_;
  }

  bool operator<(const SwarmKarmaPeer &other) const
  {
    auto now = GetCurrentTime();
    return ComputeKarmaForTime(karma_, karmaTime_, now) >
           ComputeKarmaForTime(other.karma_, other.karmaTime_, now);
  }

  friend void swap(SwarmKarmaPeer &a, SwarmKarmaPeer &b)
  {
    std::swap(a.location_, b.location_);
    std::swap(a.karma_, b.karma_);
    std::swap(a.karmaTime_, b.karmaTime_);
  }

protected:
  SwarmPeerLocation location_;
  volatile double   karma_;
  time_t            karmaTime_;

  static std::function<time_t()> toGetCurrentTime_;
};
}  // namespace swarm
}  // namespace fetch

