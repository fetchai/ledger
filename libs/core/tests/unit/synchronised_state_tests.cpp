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

#include "core/threading/synchronised_state.hpp"

#include "gmock/gmock.h"

#include <cstdint>
#include <thread>

namespace {

using namespace fetch;
using namespace testing;

class SynchronisedStateTests : public Test
{
public:
  SynchronisedState<uint32_t> waitable{0u};
};

TEST_F(SynchronisedStateTests, WaitFor_returns_when_the_condition_is_true)
{
  auto check = [this]() -> void {
    for (auto i = 0; i < 10000; ++i)
    {
      waitable.Wait([](auto const &number) -> bool { return number > 5000; });
      waitable.Apply([](auto const &number) -> void { ASSERT_THAT(number, Gt(5000)); });
    }
  };

  auto increment = [this]() -> void {
    for (auto i = 0; i < 10000; ++i)
    {
      waitable.Apply([](auto &number) -> void { ++number; });
    }
  };

  auto check_thread     = std::thread(std::move(check));
  auto increment_thread = std::thread(std::move(increment));

  check_thread.join();
  increment_thread.join();
}

}  // namespace
