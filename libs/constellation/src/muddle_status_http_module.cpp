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

#include "constellation/muddle_status_http_module.hpp"

#include "http/json_response.hpp"
#include "muddle/muddle_status.hpp"

namespace fetch {
namespace constellation {

MuddleStatusModule::MuddleStatusModule()
{
  Get("/api/status/muddle", "Returns the status of the muddle instances present on the node",
      [](http::ViewParameters const &, http::HTTPRequest const &request) {
        auto const &params = request.query();

        std::string network_name{};
        if (params.Has("network"))
        {
          network_name = static_cast<std::string>(params["network"]);
        }

        return http::CreateJsonResponse(muddle::GetStatusSummary(network_name));
      });
}

}  // namespace constellation
}  // namespace fetch
