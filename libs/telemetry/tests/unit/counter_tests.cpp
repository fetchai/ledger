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

#include "telemetry/counter.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::telemetry::Counter;

using Labels = Counter::Labels;
using CounterPtr = std::unique_ptr<Counter>;

class CounterTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    counter_ = std::make_unique<Counter>("test_counter", "Simple test counter", Labels{{"foo", "bar"}});
  }

  void TearDown() override
  {
    counter_.reset();
  }

  CounterPtr counter_;
};

TEST_F(CounterTests, SimpleCheck)
{
  EXPECT_EQ(0, counter_->count());
  ++(*counter_);
  EXPECT_EQ(1, counter_->count());
}

TEST_F(CounterTests, Increment)
{
  EXPECT_EQ(0, counter_->count());
  counter_->increment();
  EXPECT_EQ(1, counter_->count());
}

TEST_F(CounterTests, Add)
{
  EXPECT_EQ(0, counter_->count());
  counter_->add(200);
  EXPECT_EQ(200, counter_->count());
}

TEST_F(CounterTests, CheckSerialisation)
{
  EXPECT_EQ(0, counter_->count());
  counter_->add(500);

  std::ostringstream oss;
  counter_->ToStream(oss);

  static char const *EXPECTED_TEXT = R"(# HELP test_counter Simple test counter
# TYPE test_counter counter
test_counter{foo="bar"} 500
)";

  EXPECT_EQ(oss.str(), std::string{EXPECTED_TEXT});
}

}  // namespace
