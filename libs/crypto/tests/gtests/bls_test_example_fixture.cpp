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

#include "gtest/gtest.h"

class ThisFixtureClass : public ::testing::Test
{
protected:
  void SetUp() override
  {
    fixture_state_ = 9;
  }

  void TearDown() override
  {}

  // Classes and other state here etc.
  int fixture_state_ = 0;
};

TEST_F(ThisFixtureClass, BasicTestOne)
{
  ASSERT_EQ(fixture_state_, 9);
}
