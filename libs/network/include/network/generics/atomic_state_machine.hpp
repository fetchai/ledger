#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

/**
 * This is a representation of a set of states (passed in as templ
 * param) and the allowed transitions between them forming a directed
 * graph of state transitions. There is a void(void) function which
 * can be repeatedly called to drive an inheriting class's state
 * around the graph (via a thread or a polled loop or similar). It
 * calls "bool PossibleNewState(STATE &currentstate)" which can be
 * overridden to evaluate a new state. (This provides a testable
 * interface).
 *
 * The state is stored in an atomic var so that reading it/writing it
 * can be lockless and therefore freerunning.
 *
 * We throw on invalid state changes unless a specific "Force"
 * operation is used to set the state overriding the allowed
 * operations restriction.
 *
 * @tparam STATE An enum class which represents all possible states in
 * the graph.
 */

template <typename STATE>
class AtomicStateMachine
{
public:
  struct Transition
  {
    STATE to;
    STATE from;

    Transition(STATE t, STATE f)
      : to(t)
      , from(f)
    {}

    bool operator<(Transition const &other) const
    {
      if (to < other.to)
      {
        return true;
      }
      if (to > other.to)
      {
        return false;
      }
      if (from < other.from)
      {
        return true;
      }
      if (from > other.from)
      {
        return false;
      }
      return false;
    }
  };

  AtomicStateMachine()          = default;
  virtual ~AtomicStateMachine() = default;

  AtomicStateMachine(std::initializer_list<Transition> transitions)
  {
    allowed_.insert(transitions.begin(), transitions.end());
  }

  AtomicStateMachine &Allow(STATE new_state, STATE old_state)
  {
    allowed_.insert(Transition(new_state, old_state));
    return *this;
  }

  bool Set(STATE new_state)
  {
    auto old_state = state_.exchange(new_state);
    if (old_state == new_state)
    {
      return false;
    }
    auto txn = Transition(new_state, old_state);
    if (allowed_.find(txn) == allowed_.end())
    {
      FETCH_LOG_WARN("AtomicStateMachine<>", "transition not allowed." +
                                                 std::to_string(int(old_state)) + "->" +
                                                 std::to_string(int(new_state)));
      throw std::range_error("transition not allowed.");
    }
    return true;
  }

  bool Set(STATE new_state, STATE expected)
  {
    return state_.compare_exchange_strong(expected, new_state);
  }

  bool Force(STATE new_state)
  {
    auto current = state_.exchange(new_state);
    return current != new_state;
  }

  STATE Get() const
  {
    return state_.load();
  }

  void Work()
  {
    STATE curstate = Get();
    if (PossibleNewState(curstate))
    {
      Set(curstate);
    }
  }

  /* Return a new state or 0 for no change.
   */
  virtual bool PossibleNewState(STATE & /*currentstate*/)
  {
    return false;
  }

private:
  std::atomic<STATE>   state_{STATE::INITIAL};
  std::set<Transition> allowed_;
};

}  // namespace network
}  // namespace fetch
