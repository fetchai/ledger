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

#include "moment/clocks.hpp"

#include "gtest/gtest.h"

#include <chrono>
#include <memory>

TEST(ClockTests, BasicChecks)
{
  auto test_clock = fetch::moment::CreateAdjustableClock("default");
  auto prod_clock = fetch::moment::GetClock("default");

  EXPECT_EQ(prod_clock.get(), test_clock.get());

  auto const start = prod_clock->Now();
  test_clock->Advance(std::chrono::hours{1});
  auto const delta = prod_clock->Now() - start;

  EXPECT_GE(delta, std::chrono::hours{1});
}
