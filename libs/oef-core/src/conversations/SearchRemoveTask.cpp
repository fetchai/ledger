#include "oef-core/conversations/SearchRemoveTask.hpp"
#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/utils/OefUri.hpp"
#include "oef-core/conversations/SearchRemoveTask.hpp"
#include "oef-messages/dap_interface.hpp"

static Counter remove_task_created("mt-core.search.remove.tasks_created");
static Counter remove_task_errored("mt-core.search.remove.tasks_errored");
static Counter remove_task_succeeded("mt-core.search.remove.tasks_succeeded");

SearchRemoveTask::EntryPoint searchRemoveTaskEntryPoints[] = {
    &SearchRemoveTask::createConv,
    &SearchRemoveTask::handleResponse,
};

SearchRemoveTask::SearchRemoveTask(std::shared_ptr<SearchRemoveTask::IN_PROTO> initiator,
                                   std::shared_ptr<OutboundConversations>      outbounds,
                                   std::shared_ptr<OefAgentEndpoint> endpoint, uint32_t msg_id,
                                   std::string core_key, std::string agent_uri, bool remove_row)
  : SearchConversationTask("remove", std::move(initiator), std::move(outbounds),
                           std::move(endpoint), msg_id, std::move(core_key), std::move(agent_uri),
                           searchRemoveTaskEntryPoints, this)
  , remove_row_(remove_row)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Task created.");
  remove_task_created++;
}

SearchRemoveTask::~SearchRemoveTask()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Task gone.");
}

SearchRemoveTask::StateResult SearchRemoveTask::handleResponse(void)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Woken ");
  FETCH_LOG_INFO(LOGGING_NAME, "Response.. ", conversation->getAvailableReplyCount());

  if (conversation->getAvailableReplyCount() == 0)
  {
    remove_task_errored++;
    return SearchRemoveTask::StateResult(0, ERRORED);
  }

  auto response = std::static_pointer_cast<Successfulness>(conversation->getReply(0));

  // TODO should add a status answer, even in the case of no error

  if (!response->success())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Error response from search, code: ", response->errorcode(),
                   ", narrative:");
    for (const auto &n : response->narrative())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "  ", n);
    }
    std::shared_ptr<OUT_PROTO> answer = std::make_shared<OUT_PROTO>();
    answer->set_answer_id(msg_id_);
    auto error = answer->mutable_oef_error();
    error->set_operation(fetch::oef::pb::Server_AgentMessage_OEFError::UNREGISTER_SERVICE);
    FETCH_LOG_WARN(LOGGING_NAME, "Sending error ", error, " to ", agent_uri_);

    remove_task_errored++;

    if (sendReply)
    {
      sendReply(answer, endpoint);
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "No sendReply!!");
    }
  }
  else
  {
    remove_task_succeeded++;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "COMPLETE");

  return SearchRemoveTask::StateResult(0, COMPLETE);
}

std::shared_ptr<SearchRemoveTask::REQUEST_PROTO> SearchRemoveTask::make_request_proto()
{
  auto remove = std::make_shared<fetch::oef::pb::Remove>();
  remove->set_key(core_key_);
  remove->set_all(remove_row_);
  if (remove_row_)
  {
    remove->set_agent_key(agent_uri_);
  }
  if (initiator)
  {
    remove->mutable_model()->CopyFrom(initiator->description().model());
    remove->mutable_values()->CopyFrom(initiator->description().values());
    OEFURI::URI uri;
    uri.coreKey = core_key_;
    uri.parseAgent(agent_uri_);
    uri.empty           = false;
    std::string row_key = uri.toString();
    remove->mutable_service_description()->CopyFrom(initiator->description_v2());
    for (auto &cqo : *(remove->mutable_service_description()->mutable_actions()))
    {
      cqo.set_row_key(row_key);
    }
  }
  return remove;
}
