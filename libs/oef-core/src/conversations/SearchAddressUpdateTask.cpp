#include "oef-core/conversations/SearchAddressUpdateTask.hpp"
#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/utils/OefUri.hpp"
#include "oef-core/conversations/SearchAddressUpdateTask.hpp"

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

  if (conversation->getAvailableReplyCount() == 0)
  {
    return SearchAddressUpdateTask::StateResult(0, ERRORED);
  }

  auto resp = conversation->getReply(0);
  if (!resp)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Got nullptr as reply");
    return SearchAddressUpdateTask::StateResult(0, ERRORED);
  }
  auto response = std::static_pointer_cast<Successfulness>(resp);

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

  OEFURI::URI uri;
  uri.coreKey         = initiator->key();
  uri.empty           = false;
  std::string row_key = uri.toString();

  fetch::oef::pb::Update_DataModelInstance *dm = update->add_data_models();
  auto action                                  = dm->mutable_service_description()->add_actions();

  action->set_row_key(row_key);
  action->set_query_field_type("address");
  auto value = action->mutable_query_field_value();
  value->set_typecode("address");
  value->mutable_addr()->CopyFrom(*initiator);

  FETCH_LOG_INFO(LOGGING_NAME, "Sending core address to search: ", update->DebugString());

  return update;
}
