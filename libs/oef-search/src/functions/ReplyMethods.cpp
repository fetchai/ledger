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

#include "oef-search/functions/ReplyMethods.hpp"

void SendExceptionReply(const std::string &where, const Uri &uri, const std::exception &e,
                        std::shared_ptr<OefSearchEndpoint> &endpoint)
{
  auto status = std::make_shared<Successfulness>();
  status->set_success(false);
  status->add_narrative(e.what());
  SendReply<Successfulness>("Exception@" + where, uri, std::move(status), endpoint);
}

void SendErrorReply(const std::string &message, const Uri &uri,
                    std::shared_ptr<OefSearchEndpoint> &endpoint)
{
  auto status = std::make_shared<Successfulness>();
  status->set_success(false);
  status->add_narrative(message);
  SendReply<Successfulness>("Error: " + message, uri, std::move(status), endpoint);
}
