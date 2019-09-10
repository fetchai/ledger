#pragma once

#include "mt-core/tasks-base/src/cpp/IMtCoreTask.hpp"
#include "mt-core/comms/src/cpp/OefAgentEndpoint.hpp"
#include "mt-core/agents/src/cpp/Agents.hpp"
#include "mt-core/tasks/src/cpp/utils.hpp"
#include "mt-core/agents/src/cpp/Agent.hpp"
#include "fetch_teams/ledger/logger.hpp"


class OefEndpoint;

template<class PROTOBUF>
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

  AgentToAgentMessageTask(AgentP sourceAgent, int32_t message_id, ProtoP pb, AgentsP agents)
  : pb_{std::move(pb)}
  {
    OEFURI::URI uri;
    uri.parseAgent(pb_->destination());
    agent_ = agents->find(uri.agentKey);

    if (agent_ == nullptr) {
      create_dialouge_error(message_id);
      agent_ = sourceAgent;
    } else {
      create_message(message_id, std::move(uri), sourceAgent->getPublicKey());
    }

    source_key_ = sourceAgent->getPublicKey();

    FETCH_LOG_INFO(LOGGING_NAME, "Message to ", agent_->getPublicKey(), " from ", source_key_,
        ": ", message_pb_->DebugString());
  }

  virtual ~AgentToAgentMessageTask()
  {
  }

  void create_message(int32_t message_id, OEFURI::URI uri, const std::string& public_key)
  {
    message_pb_ = std::make_shared<Message>();
    uint32_t did = pb_->dialogue_id();
    message_pb_->set_answer_id(message_id);
    message_pb_->set_source_uri(pb_->source_uri());
    message_pb_->set_target_uri(pb_->target_uri());
    if (message_pb_->target_uri().size()==0) {
      message_pb_->set_target_uri(uri.toString());
    }
    auto content = message_pb_->mutable_content();
    content->set_dialogue_id(did);
    content->set_origin(public_key);
    if(pb_->has_content()) {
      content->set_allocated_content(pb_->release_content());
    }
    if(pb_->has_fipa()) {
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

  virtual bool isRunnable(void) const
  {
    return true;
  }

  virtual ExitState run(void)
  {
    // TODO(kll): it's possible there's a race hazard here. Need to think about this.
    if (agent_ -> send(message_pb_) . Then([this](){ this -> makeRunnable(); }). Waiting())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Defer message send...");
      return ExitState::DEFER;
    }

    agent_ -> run_sending();
    FETCH_LOG_INFO(LOGGING_NAME, "Message sent to: ", agent_->getPublicKey(), " from: ", source_key_);
    pb_.reset();
    message_pb_.reset();
    return ExitState::COMPLETE;
  }

protected:
  AgentP agent_;
private:
  ProtoP pb_;
  MessageP message_pb_;
  std::string source_key_;

  AgentToAgentMessageTask(const AgentToAgentMessageTask &other) = delete;
  AgentToAgentMessageTask &operator=(const AgentToAgentMessageTask &other) = delete;
  bool operator==(const AgentToAgentMessageTask &other) = delete;
  bool operator<(const AgentToAgentMessageTask &other) = delete;
};

//namespace std { template<> void swap(TSendProto& lhs, TSendProto& rhs) { lhs.swap(rhs); } }
//std::ostream& operator<<(std::ostream& os, const TSendProto &output) {}
