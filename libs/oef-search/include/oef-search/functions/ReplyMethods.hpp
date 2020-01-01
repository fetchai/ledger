#pragma once
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

#include "logging/logging.hpp"
#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-search/comms/OefSearchEndpoint.hpp"

#include <exception>
#include <memory>
#include <string>

template <typename PROTO>
void SendReply(const std::string &log_message, const Uri &uri, std::shared_ptr<PROTO> response,
               std::shared_ptr<OefSearchEndpoint> &endpoint)
{
  try
  {
    FETCH_LOG_INFO("SendReply", log_message, " ", response->DebugString(), ", PATH: ");
    uri.diagnostic();
    auto resp_pair = std::make_pair(uri, response);
    auto reply_sender =
        std::make_shared<TSendProtoTask<OefSearchEndpoint, std::pair<Uri, std::shared_ptr<PROTO>>>>(
            resp_pair, endpoint);
    reply_sender->submit();
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_ERROR("SendReply", "Failed to send reply: ", e.what());
  }
}

void SendExceptionReply(const std::string &where, const Uri &uri, const std::exception &e,
                        std::shared_ptr<OefSearchEndpoint> &endpoint);
void SendErrorReply(const std::string &message, const Uri &uri,
                    std::shared_ptr<OefSearchEndpoint> &endpoint);
