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

#include "core/reactor.hpp"
#include "core/state_machine.hpp"

#include "gmock/gmock.h"

#include <utility>

using namespace fetch;
using namespace testing;

class ReactorTests : public Test
{
public:
  enum class State : uint8_t
  {
    A,
    B,
    C
  };

  ReactorTests()
    : reactor_{"Reactor"}
  {
    reactor_.ExecutionTooLongMs()   = 50;
    reactor_.ThreadWatcherCheckMs() = 200;
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
    state_seen_ = static_cast<uint8_t>(State::A);
    return State::B;
  }

  State OnB()
  {
    state_seen_ = static_cast<uint8_t>(State::B);
    return State::C;
  }

  State OnC()
  {
    state_seen_ = static_cast<uint8_t>(State::C);
    return State::A;
  }

  State OnSlowC()
  {
    state_seen_ = static_cast<uint8_t>(State::C);
    std::this_thread::sleep_for(std::chrono::milliseconds(reactor_.ExecutionTooLongMs()));
    return State::A;
  }

  State OnVerySlowC()
  {
    state_seen_ = static_cast<uint8_t>(State::C);
    std::this_thread::sleep_for(std::chrono::milliseconds(reactor_.ThreadWatcherCheckMs() * 3));
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
  StateMachinePtr      state_machine_;
  core::Reactor        reactor_;
  std::atomic<uint8_t> state_seen_{std::numeric_limits<uint8_t>::max()};
};

// Basic test - does the reactor drive the state machine through all states
TEST_F(ReactorTests, ReactorPassesThroughStates)
{
  // Note: because it is a fixture you need to upcast the this pointer
  // clang-format off
  state_machine_->RegisterHandler(State::A, static_cast<ReactorTests *>(this), &ReactorTests::OnA);
  state_machine_->RegisterHandler(State::B, static_cast<ReactorTests *>(this), &ReactorTests::OnB);
  state_machine_->RegisterHandler(State::C, static_cast<ReactorTests *>(this), &ReactorTests::OnC);
  // clang-format on

  reactor_.Attach(state_machine_);
  reactor_.Start();

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  EXPECT_NE(state_seen_, std::numeric_limits<uint8_t>::max());
  EXPECT_EQ(reactor_.ExecutionsTooLongCounter(), 0);
  EXPECT_EQ(reactor_.ExecutionsWayTooLongCounter(), 0);
}

// Test the state machine registers states that are taking too long
TEST_F(ReactorTests, ReactorNoticesTooLongStates)
{

  // Note: because it is a fixture you need to upcast the this pointer
  // clang-format off
  state_machine_->RegisterHandler(State::A, static_cast<ReactorTests *>(this), &ReactorTests::OnA);
  state_machine_->RegisterHandler(State::B, static_cast<ReactorTests *>(this), &ReactorTests::OnB);
  state_machine_->RegisterHandler(State::C, static_cast<ReactorTests *>(this), &ReactorTests::OnSlowC);
  // clang-format on

  reactor_.Attach(state_machine_);
  reactor_.Start();

  std::this_thread::sleep_for(std::chrono::milliseconds(reactor_.ExecutionTooLongMs() * 10));

  EXPECT_NE(state_seen_, std::numeric_limits<uint8_t>::max());
  EXPECT_NE(reactor_.ExecutionsTooLongCounter(), 0);
  EXPECT_EQ(reactor_.ExecutionsWayTooLongCounter(), 0);
}

// Test the state machine registers states that are taking *way* too long
TEST_F(ReactorTests, ReactorNoticesWayTooLongStates)
{

  // Note: because it is a fixture you need to upcast the this pointer
  // clang-format off
  state_machine_->RegisterHandler(State::A, static_cast<ReactorTests *>(this), &ReactorTests::OnA);
  state_machine_->RegisterHandler(State::B, static_cast<ReactorTests *>(this), &ReactorTests::OnB);
  state_machine_->RegisterHandler(State::C, static_cast<ReactorTests *>(this), &ReactorTests::OnVerySlowC);
  // clang-format on

  reactor_.Attach(state_machine_);
  reactor_.Start();

  std::this_thread::sleep_for(std::chrono::milliseconds(reactor_.ThreadWatcherCheckMs() * 4));

  EXPECT_NE(state_seen_, std::numeric_limits<uint8_t>::max());
  EXPECT_NE(reactor_.ExecutionsTooLongCounter(), 0);
  EXPECT_NE(reactor_.ExecutionsWayTooLongCounter(), 0);
}
