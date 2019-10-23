#pragma once

#include "oef-base/utils/Uri.hpp"
#include "logging/logging.hpp"
#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-search/comms/OefSearchEndpoint.hpp"
#include "oef-messages/dap_interface.hpp"

#include <string>
#include <memory>
#include <exception>

template <typename PROTO>
void SendReply(const std::string &log_message, const Uri &uri, std::shared_ptr<PROTO> response,
    std::shared_ptr<OefSearchEndpoint>& endpoint)
{
  try
  {
    FETCH_LOG_INFO("SendReply", log_message, " ", response->DebugString(),
                   ", PATH: ");
    uri.diagnostic();
    auto resp_pair    = std::make_pair(uri, response);
    auto reply_sender = std::make_shared<
    TSendProtoTask<OefSearchEndpoint, std::pair<Uri, std::shared_ptr<PROTO>>>>(
        resp_pair, endpoint);
    reply_sender->submit();
  }
  catch (std::exception &e)
  {
    FETCH_LOG_ERROR("SendReply", "Failed to send reply: ", e.what());
  }
}

void SendExceptionReply(const std::string &where, const Uri &uri, const std::exception &e, std::shared_ptr<OefSearchEndpoint>& endpoint);
void SendErrorReply(const std::string &message, const Uri &uri, std::shared_ptr<OefSearchEndpoint>& endpoint);