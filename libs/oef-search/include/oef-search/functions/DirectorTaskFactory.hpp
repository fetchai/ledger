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

#include "oef-base/comms/ConstCharArrayBuffer.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-base/utils/OefUri.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-messages/search_config.hpp"
#include "oef-messages/search_query.hpp"
#include "oef-messages/search_remove.hpp"
#include "oef-messages/search_update.hpp"
#include "oef-search/comms/IAddSearchPeer.hpp"
#include "oef-search/comms/OefSearchEndpoint.hpp"
#include "oef-search/dap_manager/DapManager.hpp"
#include <exception>

class OutboundConversations;

class DirectorTaskFactory : public IOefTaskFactory<OefSearchEndpoint>,
                            public std::enable_shared_from_this<DirectorTaskFactory>
{
public:
  static constexpr char const *LOGGING_NAME = "DirectorTaskFactory";

  DirectorTaskFactory(std::shared_ptr<OefSearchEndpoint>     the_endpoint,
                      std::shared_ptr<OutboundConversations> outbounds,
                      std::shared_ptr<DapManager>            dap_manager,
                      fetch::oef::pb::SearchConfig &         node_config,
                      std::shared_ptr<IAddSearchPeer>        peers)
    : IOefTaskFactory(the_endpoint, outbounds)
    , dap_manager_{std::move(dap_manager)}
    , node_config_{node_config}
    , peers_{std::move(peers)}
  {}

  virtual ~DirectorTaskFactory() = default;

  virtual void ProcessMessageWithUri(const Uri &current_uri, ConstCharArrayBuffer &data);
  // Process the message, throw exceptions if they're bad.

protected:
  virtual void EndpointClosed()
  {}

protected:
  std::shared_ptr<DapManager>     dap_manager_;
  fetch::oef::pb::SearchConfig &  node_config_;
  std::shared_ptr<IAddSearchPeer> peers_;

private:
  DirectorTaskFactory(const DirectorTaskFactory &other) = delete;
  DirectorTaskFactory &operator=(const DirectorTaskFactory &other)  = delete;
  bool                 operator==(const DirectorTaskFactory &other) = delete;
  bool                 operator<(const DirectorTaskFactory &other)  = delete;
};
