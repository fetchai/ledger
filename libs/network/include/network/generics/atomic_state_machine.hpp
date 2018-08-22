#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <atomic>
#include <set>

namespace fetch {

namespace network {

class AtomicStateMachine
{
public:

  using mutex_type = fetch::mutex::Mutex;
  using lock_type = std::lock_guard<mutex_type>;

  AtomicStateMachine()
  {
  }

  AtomicStateMachine(std::initializer_list<std::pair<int,int>> transitions)
  {
    allowed_.insert(transitions.begin(), transitions.end());
  }

  AtomicStateMachine &Allow(int new_state, int old_state)
  {
    lock_type lock(mutex_);
    allowed_.insert(std::make_pair(new_state, old_state));
    return *this;
  }

  bool Set(int new_state)
  {
    auto old_state = state_.exchange(new_state);
    if (old_state == new_state)
    {
      return false;
    }
    auto txn = std::make_pair(new_state, old_state);
    if (allowed_.find(txn) == allowed_.end())
    {
      throw std::range_error(std::to_string(old_state) + " -> " + std::to_string(new_state) + " not allowed.");
    }
    return true;
  }

  bool Set(int new_state, int expected)
  {
    int local_expected = expected;
    return state_.compare_exchange_strong(local_expected, new_state);
  }

  bool Force(int new_state)
  {
    auto current = state_.exchange(new_state);
    return current != new_state;
  }

  int Get() const
  {
    return state_.load();
  }
private:
  mutex_type mutex_{__LINE__, __FILE__};
  std::atomic<int> state_{0};
  std::set<std::pair<int, int>> allowed_;
};

}  // namespace network
}  // namespace fetch
