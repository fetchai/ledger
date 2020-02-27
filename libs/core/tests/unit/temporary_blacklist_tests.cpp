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

#include "core/containers/temporary_blacklist.hpp"

#include "gtest/gtest.h"

#include <chrono>
#include <thread>

namespace {

using namespace std::chrono_literals;

using TestedClass = fetch::core::TemporaryBlacklist<int>;

TEST(TemporaryBlacklistTests, Main)
{
  TestedClass blacklist(1s);

  blacklist.Blacklist(10);
  std::this_thread::sleep_for(500ms);
  blacklist.Blacklist(42);

  ASSERT_TRUE(blacklist.IsBlacklisted(10));
  ASSERT_TRUE(blacklist.IsBlacklisted(42));
  ASSERT_EQ(blacklist.size(), 2);

  std::this_thread::sleep_for(600ms);

  ASSERT_FALSE(blacklist.IsBlacklisted(10));
  ASSERT_TRUE(blacklist.IsBlacklisted(42));
  ASSERT_EQ(blacklist.size(), 1);

  std::this_thread::sleep_for(500ms);

  ASSERT_FALSE(blacklist.IsBlacklisted(10));
  ASSERT_FALSE(blacklist.IsBlacklisted(42));
  ASSERT_EQ(blacklist.size(), 0);
}

}  // namespace
