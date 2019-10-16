#include "oef-search/functions/ReplyMethods.hpp"

void SendExceptionReply(const std::string &where, const Uri &uri, const std::exception &e, std::shared_ptr<OefSearchEndpoint>& endpoint)
{
  auto status = std::make_shared<Successfulness>();
  status->set_success(false);
  status->add_narrative(e.what());
  SendReply<Successfulness>("Exception@" + where, uri, std::move(status), endpoint);
}