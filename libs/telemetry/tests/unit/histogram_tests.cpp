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

#include "telemetry/histogram.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <sstream>

namespace {

using fetch::telemetry::Histogram;

using HistogramPtr = std::unique_ptr<Histogram>;

class HistogramTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    histogram_ = std::make_unique<Histogram>(std::initializer_list<double>{0.2, 0.4, 0.6, 0.8},
                                             "request_time", "Test Metric");
  }

  void TearDown() override
  {
    histogram_.reset();
  }

  HistogramPtr histogram_;
};

TEST_F(HistogramTests, SimpleCheck)
{
  // add the sample values in
  histogram_->Add(0.1);
  histogram_->Add(0.4);
  histogram_->Add(0.5);
  histogram_->Add(0.5);
  histogram_->Add(0.6);
  histogram_->Add(0.7);
  histogram_->Add(10.0);

  std::ostringstream oss;
  histogram_->ToStream(oss, Histogram::StreamMode::FULL);

  static char const *EXPECTED_TEXT = R"(# HELP request_time Test Metric
# TYPE request_time histogram
request_time_bucket{le="0.200000"} 1
request_time_bucket{le="0.400000"} 2
request_time_bucket{le="0.600000"} 5
request_time_bucket{le="0.800000"} 6
request_time_bucket{le="+Inf"} 7
request_time_sum 12.8
request_time_count 7
)";
  EXPECT_EQ(oss.str(), std::string{EXPECTED_TEXT});
}

}  // namespace
