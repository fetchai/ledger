#pragma once
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

#include "http/json_response.hpp"
#include "http/module.hpp"
#include "logging/logging.hpp"
#include "telemetry/registry.hpp"

#include <sstream>

namespace fetch {

class TelemetryHttpModule : public http::HTTPModule
{
public:
  TelemetryHttpModule()
  {
    Get("/api/telemetry", "Telemetry feed.",
        [](http::ViewParameters const &, http::HTTPRequest const &) {
          static auto const TXT_MIME_TYPE = http::mime_types::GetMimeTypeFromExtension(".txt");

          // collect up the generated metrics for the system
          std::ostringstream stream;
          telemetry::Registry::Instance().Collect(stream);

          return http::HTTPResponse(stream.str(), TXT_MIME_TYPE);
        });
  }
};

}  // namespace fetch
