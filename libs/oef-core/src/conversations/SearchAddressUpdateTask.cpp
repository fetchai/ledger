#include "mt-core/conversations/src/cpp/SearchAddressUpdateTask.hpp"
#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "protos/src/protos/search_response.pb.h"

SearchAddressUpdateTask::EntryPoint searchAddressUpdateTaskEntryPoints[] = {
    &SearchAddressUpdateTask::createConv,
    &SearchAddressUpdateTask::handleResponse,
};

SearchAddressUpdateTask::SearchAddressUpdateTask(
    std::shared_ptr<SearchAddressUpdateTask::IN_PROTO> initiator,
    std::shared_ptr<OutboundConversations> outbounds, std::shared_ptr<OefAgentEndpoint> endpoint)
  : SearchConversationTask("update", std::move(initiator), std::move(outbounds),
                           std::move(endpoint), 1, "", "", searchAddressUpdateTaskEntryPoints, this)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Task created.");
}

SearchAddressUpdateTask::~SearchAddressUpdateTask()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Task gone.");
}

SearchAddressUpdateTask::StateResult SearchAddressUpdateTask::handleResponse(void)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Woken ");
  FETCH_LOG_INFO(LOGGING_NAME, "Response.. ", conversation->getAvailableReplyCount());

  auto response =
      std::static_pointer_cast<fetch::oef::pb::UpdateResponse>(conversation->getReply(0));

  if (sendReply)
  {
    sendReply(response, endpoint);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "No sendReply!!");
  }

  FETCH_LOG_INFO(LOGGING_NAME, "COMPLETE");

  return SearchAddressUpdateTask::StateResult(0, COMPLETE);
}

std::shared_ptr<SearchAddressUpdateTask::REQUEST_PROTO>
SearchAddressUpdateTask::make_request_proto()
{
  auto update = std::make_shared<fetch::oef::pb::Update>();
  update->set_key(initiator->key());
  fetch::oef::pb::Update_Attribute *attr = update->add_attributes();
  attr->set_name(fetch::oef::pb::Update_Attribute_Name::Update_Attribute_Name_NETWORK_ADDRESS);
  auto *val = attr->mutable_value();
  val->set_type(10);
  val->mutable_a()->CopyFrom(*initiator);
  FETCH_LOG_INFO(LOGGING_NAME, "Sending core address to search: ", update->DebugString());
  return update;
}
