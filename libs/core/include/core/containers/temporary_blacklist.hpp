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

#include "core/assert.hpp"
#include "core/containers/append.hpp"
#include "core/containers/is_in.hpp"
#include "core/mutex.hpp"
#include "moment/clock_interfaces.hpp"
#include "moment/clocks.hpp"

#include <chrono>
#include <deque>
#include <unordered_set>
#include <utility>

namespace fetch {
namespace core {

using namespace std::chrono_literals;

constexpr char const *BLACKLIST_CLOCK_NAME = "core:TemporaryBlacklist";

template <class T>
class TemporaryBlacklist
{
public:
  using type     = T;
  using Duration = moment::ClockInterface::Duration;

  TemporaryBlacklist() = default;
  explicit TemporaryBlacklist(Duration cooldown_period)
    : cooldown_period_(cooldown_period)
  {}

  TemporaryBlacklist(TemporaryBlacklist const &) = delete;
  TemporaryBlacklist(TemporaryBlacklist &&)      = delete;

  TemporaryBlacklist &operator=(TemporaryBlacklist const &) = delete;
  TemporaryBlacklist &operator=(TemporaryBlacklist &&) = delete;

  void Blacklist(T t)
  {
    auto now = Now();
    Cleanup(now);
    Append(chronology_, now, t);
    Append(blacklisted_, std::move(t));
  }

  bool IsBlacklisted(T const &t) const
  {
    Cleanup(Now());
    return IsIn(blacklisted_, t);
  }

  std::size_t size() const
  {
    Cleanup(Now());
    return blacklisted_.size();
  }

private:
  using Timestamp   = moment::ClockInterface::Timestamp;
  using Chronology  = std::deque<std::pair<Timestamp, type>>;
  using Blacklisted = std::unordered_set<type>;

  static Timestamp Now()
  {
    static moment::ClockPtr clock = assert::True(moment::GetClock(BLACKLIST_CLOCK_NAME),
                                                 "Could not initialize blacklist timer");
    return clock->Now();
  }

  void Cleanup(Timestamp t) const
  {
    t -= cooldown_period_;
    auto chrono_it = chronology_.begin();
    while (chrono_it != chronology_.end() && chrono_it->first <= t)
    {
      blacklisted_.erase(chrono_it->second);
      ++chrono_it;
    }
    chronology_.erase(chronology_.begin(), chrono_it);
  }

  mutable Chronology  chronology_;
  mutable Blacklisted blacklisted_;
  const Duration      cooldown_period_ = 5s;
};

}  // namespace core
}  // namespace fetch
