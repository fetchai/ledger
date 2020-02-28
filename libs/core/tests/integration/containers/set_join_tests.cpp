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

#include "core/containers/set_join.hpp"
#include "meta/containers/set_element.hpp"

#include "gtest/gtest.h"

using namespace fetch;

namespace {

TEST(SetJoinTests, BasicSetCheck)
{
  std::unordered_set<int> a = {1, 2, 3, 4, 5, 6};
  std::unordered_set<int> b = {3, 4, 5, 6, 7, 8};

  auto const c = a + b;

  EXPECT_EQ(c.size(), 8);
  EXPECT_TRUE(meta::IsIn(c, 1));
  EXPECT_TRUE(meta::IsIn(c, 2));
  EXPECT_TRUE(meta::IsIn(c, 3));
  EXPECT_TRUE(meta::IsIn(c, 4));
  EXPECT_TRUE(meta::IsIn(c, 5));
  EXPECT_TRUE(meta::IsIn(c, 6));
  EXPECT_TRUE(meta::IsIn(c, 7));
  EXPECT_TRUE(meta::IsIn(c, 8));
}

}  // namespace
