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

#include "telemetry/histogram_map.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::telemetry::HistogramMap;
using fetch::telemetry::OutputStream;

using HistogramMapPtr = std::unique_ptr<HistogramMap>;

class HistogramMapTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    using B = std::vector<double>;

    histogram_map_ = std::make_unique<HistogramMap>("http_requests", "path", B{0.2, 0.4, 0.6, 0.8},
                                                    "Request time for HTTP paths");
  }

  void TearDown() override
  {
    histogram_map_.reset();
  }

  HistogramMapPtr histogram_map_;
};

TEST_F(HistogramMapTests, SimpleCheck)
{
  // add the sample values in
  histogram_map_->Add("/", 0.1);
  histogram_map_->Add("/", 0.4);
  histogram_map_->Add("/", 0.5);
  histogram_map_->Add("/", 0.5);
  histogram_map_->Add("/", 0.6);
  histogram_map_->Add("/", 0.7);
  histogram_map_->Add("/", 10.0);

  histogram_map_->Add("/status", 0.5);
  histogram_map_->Add("/status", 0.5);
  histogram_map_->Add("/status", 0.6);
  histogram_map_->Add("/status", 0.7);
  histogram_map_->Add("/status", 0.7);

  std::ostringstream oss;
  OutputStream stream{oss};
  histogram_map_->ToStream(stream);

  static char const *EXPECTED_TEXT = R"(# HELP http_requests Request time for HTTP paths
# TYPE http_requests histogram
http_requests_bucket{path="/status",le="0.200000"} 0
http_requests_bucket{path="/status",le="0.400000"} 0
http_requests_bucket{path="/status",le="0.600000"} 3
http_requests_bucket{path="/status",le="0.800000"} 5
http_requests_bucket{path="/status",le="+Inf"} 5
http_requests_sum{path="/status"} 3
http_requests_count{path="/status"} 5
http_requests_bucket{path="/",le="0.200000"} 1
http_requests_bucket{path="/",le="0.400000"} 2
http_requests_bucket{path="/",le="0.600000"} 5
http_requests_bucket{path="/",le="0.800000"} 6
http_requests_bucket{path="/",le="+Inf"} 7
http_requests_sum{path="/"} 12.8
http_requests_count{path="/"} 7
)";

  EXPECT_EQ(oss.str(), std::string{EXPECTED_TEXT});
}

}  // namespace
