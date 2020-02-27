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

#include "core/containers/is_in.hpp"
#include "core/mutex.hpp"

#include <chrono>
#include <deque>
#include <unordered_set>
#include <utility>

namespace fetch {
namespace core {

using namespace std::chrono_literals;

template <class T>
class TemporaryBlacklist
{
public:
  using type     = T;
  using Clock    = std::chrono::steady_clock;
  using Duration = Clock::duration;

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
    auto now = Clock::now();
    Cleanup(now);
    chronology_.emplace(now, t);
    blacklisted_.insert(std::move(t));
  }

  bool IsBlacklisted(T const &t) const
  {
    Cleanup(Clock::now());
    return IsIn(blacklisted_, t);
  }

  std::size_t size() const
  {
    Cleanup(Clock::now());
    return blacklisted_.size();
  }

private:
  using TimePoint   = Clock::time_point;
  using Chronology  = std::deque<std::pair<TimePoint, type>>;
  using Blacklisted = std::unordered_set<type>;

  void Cleanup(TimePoint t) const
  {
    t -= cooldown_period_;
    auto crhono_it = chronology.begin();
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
