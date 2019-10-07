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
#include <exception>

class OutboundConversations;

class SearchTaskFactory : public IOefTaskFactory<OefSearchEndpoint>,
                          public std::enable_shared_from_this<SearchTaskFactory>
{
public:
  static constexpr char const *LOGGING_NAME = "SearchTaskFactory";

  SearchTaskFactory(std::shared_ptr<OefSearchEndpoint>     the_endpoint,
                    std::shared_ptr<OutboundConversations> outbounds,
                    std::shared_ptr<DapManager>            dap_manager)
    : IOefTaskFactory(the_endpoint, outbounds)
    , dap_manager_{std::move(dap_manager)}
  {}

  virtual ~SearchTaskFactory()
  {}

  template <typename PROTO>
  void SendReply(const std::string &log_message, const Uri &uri, std::shared_ptr<PROTO> response)
  {
    try
    {
      FETCH_LOG_INFO(LOGGING_NAME, "SendReply: ", log_message, " ", response->DebugString(),
                     ", PATH: ");
      uri.diagnostic();
      auto resp_pair    = std::make_pair(uri, response);
      auto reply_sender = std::make_shared<
          TSendProtoTask<OefSearchEndpoint, std::pair<Uri, std::shared_ptr<PROTO>>>>(
          resp_pair, this->endpoint);
      reply_sender->submit();
    }
    catch (std::exception &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to send reply: ", e.what());
    }
  }

  void SendExceptionReply(const std::string &where, const Uri &uri, const std::exception &e)
  {
    auto status = std::make_shared<Successfulness>();
    status->set_success(false);
    status->add_narrative(e.what());
    SendReply<Successfulness>("Exception@" + where, uri, std::move(status));
  }

  virtual void ProcessMessageWithUri(const Uri &current_uri, ConstCharArrayBuffer &data);
  // Process the message, throw exceptions if they're bad.

protected:
  virtual void EndpointClosed(void)
  {}

  void HandleQuery(const fetch::oef::pb::SearchQuery &query, const Uri &current_uri);

protected:
  std::shared_ptr<DapManager> dap_manager_;

private:
  SearchTaskFactory(const SearchTaskFactory &other) = delete;
  SearchTaskFactory &operator=(const SearchTaskFactory &other)  = delete;
  bool               operator==(const SearchTaskFactory &other) = delete;
  bool               operator<(const SearchTaskFactory &other)  = delete;
};
