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
#include <map>
#include <unordered_set>

namespace fetch {
namespace core {

template <class T, std::size_t cooldown_ms = 5000>
class TemporaryBlacklist
{
public:
  using type = T;

  TemporaryBlacklist()                           = default;
  TemporaryBlacklist(TemporaryBlacklist const &) = delete;
  TemporaryBlacklist(TemporaryBlacklist &&)      = delete;

  TemporaryBlacklist &operator=(TemporaryBlacklist const &) = delete;
  TemporaryBlacklist &operator=(TemporaryBlacklist &&) = delete;

  void Blacklist(T t)
  {
    FETCH_LOCK(lock_);
    auto now = Clock::now();
    Cleanup(now);
    chronology_.emplace(now, t);
    blacklisted_.insert(std::move(t));
  }

  bool IsBlacklisted(T const &t) const
  {
    FETCH_LOCK(lock_);
    Cleanup(Clock::now());
    return IsIn(blacklisted_, t);
  }

  std::size_t size() const
  {
    FETCH_LOCK(lock_);
    Cleanup(Clock::now());
    return blacklisted_.size();
  }

private:
  using Clock     = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration  = Clock::duration;

  using Chronology  = std::map<TimePoint, type>;
  using Blacklisted = std::unordered_set<type>;

  static constexpr Duration COOLDOWN_PERIOD = std::chrono::milliseconds(cooldown_ms);

  void Cleanup(TimePoint t) const
  {
    t -= COOLDOWN_PERIOD;
    auto earliest_surviving_record = chronology_.upper_bound(t);
    for (auto chrono_it = chronology_.begin(); chrono_it != earliest_surviving_record; ++chrono_it)
    {
      blacklisted_.erase(chrono_it->second);
    }
    chronology_.erase(chronology_.begin(), earliest_surviving_record);
  }

  mutable Chronology  chronology_;
  mutable Blacklisted blacklisted_;
  mutable Mutex       lock_;
};

}  // namespace core
}  // namespace fetch
