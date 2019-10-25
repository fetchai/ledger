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

#include "http/module.hpp"
#include "messenger/messenger_api.hpp"

namespace fetch {
namespace messenger {

class MessengerHttpModule : public http::HTTPModule
{
public:
  MessengerHttpModule(MessengerAPI &messenger)
    : messenger_{messenger}
  {

    Get("/api/messenger/register", "Registers an agent to the network.",
        [](http::ViewParameters const &, http::HTTPRequest const &) {
          return http::CreateJsonResponse("{}");
        });

    Get("/api/messenger/unregister", "Unregisters an agent from the network.",
        [](http::ViewParameters const &, http::HTTPRequest const &) {
          return http::CreateJsonResponse("{}");
        });

    Get("/api/messenger/sendmessage", "Sends a message to a specific agent.",
        [](http::ViewParameters const &, http::HTTPRequest const &) {
          return http::CreateJsonResponse("{}");
        });

    Get("/api/messenger/getmessages", "Gets messages in inbox.",
        [](http::ViewParameters const &, http::HTTPRequest const &) {
          return http::CreateJsonResponse("{}");
        });

    Get("/api/messenger/findagent", "Finds agents matching search criteria.",
        [](http::ViewParameters const &, http::HTTPRequest const &) {
          return http::CreateJsonResponse("{}");
        });

    Get("/api/messenger/advertise", "Creates advertisement on the node.",
        [](http::ViewParameters const &, http::HTTPRequest const &) {
          return http::CreateJsonResponse("{}");
        });
  }

private:
  MessengerAPI &messenger_;
};
}  // namespace messenger
}  // namespace fetch
