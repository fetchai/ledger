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

#include "core/logging.hpp"
#include "http/middleware/telemetry.hpp"
#include "telemetry/counter_map.hpp"
#include "telemetry/histogram_map.hpp"
#include "telemetry/registry.hpp"

#include <memory>

namespace fetch {
namespace http {
namespace middleware {
namespace {

using telemetry::CounterMapPtr;
using telemetry::HistogramMapPtr;
using telemetry::Registry;

class TelemetryData
{
public:
  // Construction / Destruction
  TelemetryData()                      = default;
  TelemetryData(TelemetryData const &) = delete;
  TelemetryData(TelemetryData &&)      = delete;
  ~TelemetryData()                     = default;

  void Update(HTTPRequest const &request, HTTPResponse const &response);

  // Operators
  TelemetryData &operator=(TelemetryData const &) = delete;
  TelemetryData &operator=(TelemetryData &&) = delete;

private:
  HistogramMapPtr durations_{Registry::Instance().CreateHistogramMap(
      {0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1., 5., 10.}, "ledger_http_request_duration_seconds",
      "path", "Histogram of HTTP request latencies")};

  CounterMapPtr status_counts_{Registry::Instance().CreateCounterMap(
      "ledger_http_response_total", "Histogram of HTTP request latencies")};
};

void TelemetryData::Update(HTTPRequest const &request, HTTPResponse const &response)
{
  FETCH_UNUSED(response);

  auto const path        = static_cast<std::string>(request.uri());
  auto const status_code = std::to_string(static_cast<int>(response.status()));

  // update the duration stats
  durations_->Add(path, request.GetDuration());

  // update the counters
  status_counts_->Increment({{"path", path}, {"code", status_code}});
}

using TelemetryDataPtr = std::shared_ptr<TelemetryData>();

}  // namespace

HTTPServer::response_middleware_type Telemetry()
{
  // create the data structure
  auto data = std::make_shared<TelemetryData>();

  // return the handler
  return [data](HTTPResponse &resp, HTTPRequest const &req) { data->Update(req, resp); };
}

}  // namespace middleware
}  // namespace http
}  // namespace fetch
