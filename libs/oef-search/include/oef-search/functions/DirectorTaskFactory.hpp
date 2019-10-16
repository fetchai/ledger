#pragma once

#include "oef-base/comms/ConstCharArrayBuffer.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-base/utils/OefUri.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-messages/search_query.hpp"
#include "oef-messages/search_remove.hpp"
#include "oef-messages/search_update.hpp"
#include "oef-search/comms/OefSearchEndpoint.hpp"
#include "oef-search/dap_manager/DapManager.hpp"
#include "oef-messages/search_config.hpp"
#include <exception>

class OutboundConversations;

class DirectorTaskFactory : public IOefTaskFactory<OefSearchEndpoint>,
                          public std::enable_shared_from_this<DirectorTaskFactory>
{
public:
  static constexpr char const *LOGGING_NAME = "DirectorTaskFactory";

  DirectorTaskFactory(std::shared_ptr<OefSearchEndpoint>   the_endpoint,
                    std::shared_ptr<OutboundConversations> outbounds,
                    std::shared_ptr<DapManager>            dap_manager,
                    fetch::oef::pb::SearchConfig&          node_config)
    : IOefTaskFactory(the_endpoint, outbounds)
    , dap_manager_{std::move(dap_manager)}
    , node_config_{node_config}
  {}

  virtual ~DirectorTaskFactory()
  {}

  virtual void ProcessMessageWithUri(const Uri &current_uri, ConstCharArrayBuffer &data);
  // Process the message, throw exceptions if they're bad.

protected:
  virtual void EndpointClosed(void)
  {}


protected:
  std::shared_ptr<DapManager> dap_manager_;
  fetch::oef::pb::SearchConfig& node_config_;

private:
  DirectorTaskFactory(const DirectorTaskFactory &other) = delete;
  DirectorTaskFactory &operator=(const DirectorTaskFactory &other)  = delete;
  bool               operator==(const DirectorTaskFactory &other) = delete;
  bool               operator<(const DirectorTaskFactory &other)  = delete;
};
