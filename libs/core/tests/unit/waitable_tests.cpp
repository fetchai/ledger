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

#include "core/threading/waitable.hpp"

#include "gmock/gmock.h"

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace {

using namespace fetch;
using namespace testing;

using Semaphore = Waitable<uint32_t>;

class WaitableTests : public Test
{
public:
  Semaphore                  semaphore{3u};
  Waitable<std::vector<int>> waitable{};
};

TEST_F(WaitableTests, Wait_returns_when_the_condition_is_true)
{
  auto check = [this]() -> void {
    semaphore.Apply([](auto &count) -> void { --count; });
    semaphore.Wait([](auto const &count) -> bool { return count == 0u; });

    waitable.Wait([](auto const &payload) -> bool { return payload.size() > 9000; });
    waitable.Apply([](auto const &payload) -> void { ASSERT_THAT(payload.size(), Gt(9000)); });
  };

  auto increment = [this]() -> void {
    semaphore.Apply([](auto &count) -> void { --count; });
    semaphore.Wait([](auto const &count) -> bool { return count == 0u; });

    for (auto i = 0; i < 10000; ++i)
    {
      waitable.Apply([](auto &payload) -> void { payload.push_back(123); });
    }
  };

  auto check_thread     = std::thread(std::move(check));
  auto increment_thread = std::thread(std::move(increment));

  semaphore.Apply([](auto &count) -> void { --count; });

  increment_thread.join();
  check_thread.join();
}

TEST_F(WaitableTests,
       Wait_allows_to_specify_optional_timeout_and_returns_true_if_condition_was_true_on_return)
{
  auto const no_timeout =
      waitable.Wait([](auto const &) { return true; }, std::chrono::milliseconds{1u});

  ASSERT_TRUE(no_timeout);
}

TEST_F(
    WaitableTests,
    on_timeout_Wait_returns_even_if_condition_is_false_and_returns_false_if_return_was_due_to_timeout)
{
  auto const no_timeout =
      waitable.Wait([](auto const &) { return false; }, std::chrono::milliseconds{1u});

  ASSERT_FALSE(no_timeout);
}

}  // namespace
