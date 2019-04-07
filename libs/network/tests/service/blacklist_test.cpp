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

#include <chrono>
#include <memory>
#include <thread>

#include <gmock/gmock.h>

#include <network/generics/blackset.hpp>

class Lock
{
  int state_ = 0;

public:
  Lock &lock()
  {
    ++state_;
    return *this;
  }
  Lock &unlock()
  {
    ++state_;
    return *this;
  }
  int state() const
  {
    return state_;
  }
};

TEST(BlacklistTest, LockFree)
{
  const std::unordered_set<int>        small{0, 1}, large{0, 1, 42};
  fetch::generics::Blackset<int, void> bs(small);
  ASSERT_EQ(bs.GetBlacklisted(), small);

  bs.Blacklist(42);
  ASSERT_EQ(bs.GetBlacklisted(), large);

  bs.Whitelist(42);
  ASSERT_EQ(bs.GetBlacklisted(), small);
}

TEST(BlacklistTest, LockPaid)
{
  const std::unordered_set<int> small{0, 1}, large{0, 1, 42};
  Lock                          lck;
  ASSERT_EQ(lck.state(), 0);

  fetch::generics::Blackset<int, Lock> bs(lck, small);
  ASSERT_EQ(lck.state(), 0);
  ASSERT_EQ(bs.GetBlacklisted(), small);
  ASSERT_EQ(lck.state(), 2);

  bs.Blacklist(42);
  ASSERT_EQ(lck.state(), 4);
  ASSERT_EQ(bs.GetBlacklisted(), large);
  ASSERT_EQ(lck.state(), 6);

  bs.Whitelist(42);
  ASSERT_EQ(lck.state(), 8);
  ASSERT_EQ(bs.GetBlacklisted(), small);
  ASSERT_EQ(lck.state(), 10);
}
