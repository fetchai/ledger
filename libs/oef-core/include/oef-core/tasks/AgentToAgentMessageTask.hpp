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
#include "oef-base/utils/OefUri.hpp"
#include "oef-core/agents/Agent.hpp"
#include "oef-core/agents/Agents.hpp"
#include "oef-core/comms/OefAgentEndpoint.hpp"
#include "oef-core/tasks-base/IMtCoreTask.hpp"

class OefEndpoint;

template <class PROTOBUF>
class AgentToAgentMessageTask : public IMtCoreTask
{
public:
  using Proto    = PROTOBUF;
  using ProtoP   = std::shared_ptr<Proto>;
  using AgentP   = std::shared_ptr<Agent>;
  using AgentsP  = std::shared_ptr<Agents>;
  using Message  = fetch::oef::pb::Server_AgentMessage;
  using MessageP = std::shared_ptr<Message>;

  static constexpr char const *LOGGING_NAME = "AgentToAgentMessageTask";

  AgentToAgentMessageTask(AgentP const &sourceAgent, int32_t message_id, ProtoP pb,
                          AgentsP const &agents)
    : pb_{std::move(pb)}
  {
    OEFURI::URI uri;
    uri.ParseAgent(pb_->destination());
    agent_ = agents->find(uri.AgentKey);

    if (agent_ == nullptr)
    {
      create_dialouge_error(message_id);
      agent_ = sourceAgent;
    }
    else
    {
      create_message(message_id, uri, sourceAgent->getPublicKey());
    }

    source_key_ = sourceAgent->getPublicKey();

    FETCH_LOG_INFO(LOGGING_NAME, "Message to ", agent_->getPublicKey(), " from ", source_key_, ": ",
                   message_pb_->DebugString());
  }

  ~AgentToAgentMessageTask() override = default;

  void create_message(int32_t message_id, OEFURI::URI const &uri, std::string const &public_key)
  {
    message_pb_ = std::make_shared<Message>();
    int32_t did = pb_->dialogue_id();
    message_pb_->set_answer_id(message_id);
    message_pb_->set_source_uri(pb_->source_uri());
    message_pb_->set_target_uri(pb_->target_uri());
    if (message_pb_->target_uri().size() == 0)
    {
      message_pb_->set_target_uri(uri.ToString());
    }
    auto content = message_pb_->mutable_content();
    content->set_dialogue_id(did);
    content->set_origin(public_key);
    if (pb_->has_content())
    {
      content->set_allocated_content(pb_->release_content());
    }
    if (pb_->has_fipa())
    {
      content->set_allocated_fipa(pb_->release_fipa());
    }
  }

  void create_dialouge_error(int32_t message_id)
  {
    message_pb_ = std::make_shared<Message>();
    message_pb_->set_answer_id(message_id);
    auto *error = message_pb_->mutable_dialogue_error();
    error->set_dialogue_id(pb_->dialogue_id());
    error->set_origin(pb_->destination());
  }

  bool IsRunnable() const override
  {
    return true;
  }

  fetch::oef::base::ExitState run() override
  {
    // TODO(kll): it's possible there's a race hazard here. Need to think about this.
    if (agent_->send(message_pb_).Then([this]() { this->MakeRunnable(); }).Waiting())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Defer message send...");
      return fetch::oef::base::ExitState::DEFER;
    }

    agent_->run_sending();
    FETCH_LOG_INFO(LOGGING_NAME, "Message sent to: ", agent_->getPublicKey(),
                   " from: ", source_key_);
    pb_.reset();
    message_pb_.reset();
    return fetch::oef::base::ExitState::COMPLETE;
  }

protected:
  AgentP agent_;

private:
  ProtoP      pb_;
  MessageP    message_pb_;
  std::string source_key_;

  AgentToAgentMessageTask(const AgentToAgentMessageTask &other) = delete;
  AgentToAgentMessageTask &operator=(const AgentToAgentMessageTask &other)  = delete;
  bool                     operator==(const AgentToAgentMessageTask &other) = delete;
  bool                     operator<(const AgentToAgentMessageTask &other)  = delete;
};

// namespace std { template<> void swap(TSendProto& lhs, TSendProto& rhs) { lhs.swap(rhs); } }
// std::ostream& operator<<(std::ostream& os, const TSendProto &output) {}
