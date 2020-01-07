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

#include "oef-base/conversation/OutboundConversation.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/utils/OefUri.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-core/conversations/SearchUpdateTask.hpp"
#include "oef-messages/dap_interface.hpp"

static Counter update_task_created("mt-core.search.update.tasks_created");
static Counter update_task_errored("mt-core.search.update.tasks_errored");
static Counter update_task_succeeded("mt-core.search.update.tasks_succeeded");

SearchUpdateTask::EntryPoint searchUpdateTaskEntryPoints[] = {
    &SearchUpdateTask::CreateConversation,
    &SearchUpdateTask::HandleResponse,
};

SearchUpdateTask::SearchUpdateTask(std::shared_ptr<SearchUpdateTask::IN_PROTO> initiator,
                                   std::shared_ptr<OutboundConversations>      outbounds,
                                   std::shared_ptr<OefAgentEndpoint> endpoint, uint32_t msg_id,
                                   std::string core_key, std::string agent_uri)
  : SearchConversationTask("update", std::move(initiator), std::move(outbounds),
                           std::move(endpoint), msg_id, std::move(core_key), std::move(agent_uri),
                           searchUpdateTaskEntryPoints, this)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Task created.");
  update_task_created++;
}

SearchUpdateTask::~SearchUpdateTask()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Task gone.");
}

SearchUpdateTask::StateResult SearchUpdateTask::HandleResponse()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Woken ");
  FETCH_LOG_INFO(LOGGING_NAME, "Response.. ", conversation->GetAvailableReplyCount());

  if (conversation->GetAvailableReplyCount() == 0)
  {
    update_task_errored++;
    return SearchUpdateTask::StateResult(0, fetch::oef::base::ERRORED);
  }

  auto resp = conversation->GetReply(0);
  if (!resp)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Got nullptr as reply");
    update_task_errored++;
    return SearchUpdateTask::StateResult(0, fetch::oef::base::ERRORED);
  }
  auto response = std::static_pointer_cast<Successfulness>(resp);

  // TODO should add a status answer, even in the case of no error

  if (!response->success())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Error response from search, code: ", response->errorcode(),
                   ", narrative:");
    for (const auto &n : response->narrative())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "  ", n);
    }
    auto answer = std::make_shared<OUT_PROTO>();
    answer->set_answer_id(static_cast<int32_t>(msg_id_));
    auto error = answer->mutable_oef_error();
    error->set_operation(fetch::oef::pb::Server_AgentMessage_OEFError::REGISTER_SERVICE);
    FETCH_LOG_WARN(LOGGING_NAME, "Sending error ", error, " ", agent_uri_);
    update_task_errored++;
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
    update_task_succeeded++;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "COMPLETE");

  return SearchUpdateTask::StateResult(0, fetch::oef::base::COMPLETE);
}

std::shared_ptr<SearchUpdateTask::REQUEST_PROTO> SearchUpdateTask::make_request_proto()
{
  auto update = std::make_shared<fetch::oef::pb::Update>();
  update->set_key(core_key_);
  fetch::oef::pb::Update_DataModelInstance *dm = update->add_data_models();
  dm->set_key(agent_uri_);
  if (initiator->description().has_model())
  {
    dm->mutable_model()->CopyFrom(initiator->description().model());
  }
  dm->mutable_values()->CopyFrom(initiator->description().values());
  OEFURI::URI uri;
  uri.CoreKey = core_key_;
  uri.ParseAgent(agent_uri_);
  uri.empty           = false;
  std::string row_key = uri.ToString();
  dm->mutable_service_description()->CopyFrom(initiator->description_v2());
  for (auto &cqo : *(dm->mutable_service_description()->mutable_actions()))
  {
    cqo.set_row_key(row_key);
  }
  return update;
}
