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

#include "core/reactor.hpp"
#include "core/state_machine.hpp"

#include "gmock/gmock.h"

using namespace fetch;
using namespace testing;

class StateMachineTests : public Test
{
public:
  enum class State : uint8_t
  {
    A,
    B,
    C,
  };

  StateMachineTests()
    : reactor_{"Reactor"}
  {
  }

  using StateMachine    = core::StateMachine<State>;
  using StateMachinePtr = std::shared_ptr<StateMachine>;

  void SetUp() override
  {
    state_machine_ = std::make_shared<StateMachine>("TestStateMachine", State::A, ToString);
  }

  void TearDown() override
  {
    reactor_.Stop();
  }

  State OnA()
  {
    std::cerr << "A" << std::endl; // DELETEME_NH
    state_seen_ = static_cast<uint8_t>(State::A);
    return State::B;
  }

  State OnB()
  {
    std::cerr << "B" << std::endl; // DELETEME_NH
    state_seen_ = static_cast<uint8_t>(State::B);
    return State::C;
  }

  State OnC()
  {
    std::cerr << "C" << std::endl; // DELETEME_NH
    state_seen_ = static_cast<uint8_t>(State::C);
    return State::A;
  }

  static char const *ToString(State state)
  {
    char const *text = "unknown";

    switch (state)
    {
      case State::A:
        text = "A";
        break;
      case State::B:
        text = "B";
        break;
      case State::C:
        text = "C";
        break;
    }

    return text;
  }

protected:
  std::shared_ptr<StateMachine> state_machine_;
  core::Reactor                 reactor_;
  std::atomic<uint8_t>          state_seen_{std::numeric_limits<uint8_t>::max()};
};

TEST_F(StateMachineTests, StateMachinePassesThroughStates)
{

  // Note: because it is a fixture you need to upcast the this pointer
  // clang-format off
  state_machine_->RegisterHandler(State::A, static_cast<StateMachineTests *>(this), &StateMachineTests::OnA);
  state_machine_->RegisterHandler(State::B, static_cast<StateMachineTests *>(this), &StateMachineTests::OnB);
  state_machine_->RegisterHandler(State::C, static_cast<StateMachineTests *>(this), &StateMachineTests::OnC);
  // clang-format on

   reactor_.Attach(state_machine_);
   reactor_.Start();

   std::this_thread::sleep_for(std::chrono::milliseconds(5));

   EXPECT_NE(state_seen_, std::numeric_limits<uint8_t>::max());
}
