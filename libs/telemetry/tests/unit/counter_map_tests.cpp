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

#include "telemetry/counter_map.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::telemetry::Measurement;
using fetch::telemetry::CounterMap;

using CounterMapPtr = std::unique_ptr<CounterMap>;

class CounterMapTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    counter_map_ = std::make_unique<CounterMap>("muddle_stats", "Some test muddle stats");
  }

  void TearDown() override
  {
    counter_map_.reset();
  }

  CounterMapPtr counter_map_;
};

TEST_F(CounterMapTests, SimpleCheck)
{
  // add the sample values in
  counter_map_->Increment({{"service", "1"}, {"channel", "1"}});
  counter_map_->Increment({{"service", "1"}, {"channel", "1"}});
  counter_map_->Increment({{"service", "1"}, {"channel", "1"}});
  counter_map_->Increment({{"service", "1"}, {"channel", "1"}});

  counter_map_->Increment({{"service", "1"}, {"channel", "2"}});
  counter_map_->Increment({{"service", "1"}, {"channel", "2"}});
  counter_map_->Increment({{"service", "1"}, {"channel", "2"}});
  counter_map_->Increment({{"service", "1"}, {"channel", "2"}});
  counter_map_->Increment({{"service", "1"}, {"channel", "2"}});
  counter_map_->Increment({{"service", "1"}, {"channel", "2"}});

  std::ostringstream oss;
  counter_map_->ToStream(oss, Measurement::StreamMode::FULL);

  static char const *EXPECTED_TEXT = R"(# HELP muddle_stats Some test muddle stats
# TYPE muddle_stats counter
muddle_stats{service="1",channel="2"} 6
muddle_stats{service="1",channel="1"} 4
)";

  EXPECT_EQ(oss.str(), std::string{EXPECTED_TEXT});
}

}  // namespace
