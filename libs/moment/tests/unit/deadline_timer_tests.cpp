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

#include "moment/deadline_timer.hpp"

#include "gtest/gtest.h"

using fetch::moment::DeadlineTimer;
using fetch::moment::CreateAdjustableClock;


TEST(DeadlineTimerTests, BasicChecks)
{
  auto clock = CreateAdjustableClock("test1");

  DeadlineTimer timer{"test1"};
  timer.Restart(std::chrono::hours{3});

  EXPECT_FALSE(timer.HasExpired());
  clock->Advance(std::chrono::hours{1});

  EXPECT_FALSE(timer.HasExpired());
  clock->Advance(std::chrono::hours{1});

  EXPECT_FALSE(timer.HasExpired());
  clock->Advance(std::chrono::hours{2});

  EXPECT_TRUE(timer.HasExpired());
}