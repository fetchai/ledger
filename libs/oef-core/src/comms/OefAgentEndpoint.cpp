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

#include <utility>

#include <utility>

#include "oef-core/comms/OefAgentEndpoint.hpp"

#include "logging/logging.hpp"
#include "oef-base/comms/Endianness.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/monitoring/Gauge.hpp"
#include "oef-base/proto_comms/ProtoMessageReader.hpp"
#include "oef-base/proto_comms/ProtoMessageSender.hpp"
#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-core/karma/IKarmaPolicy.hpp"
#include "oef-core/karma/XKarma.hpp"
#include "oef-messages/agent.hpp"

static Gauge   count("mt-core.network.OefAgentEndpoint.gauge");
static Counter hb_sent("mt-core.network.OefAgentEndpoint.heartbeats.sent");
static Counter hb_recvd("mt-core.network.OefAgentEndpoint.heartbeats.recvd");
static Gauge   hb_max_os("mt-core.network.OefAgentEndpoint.heartbeats.max-outstand.gauge");

OefAgentEndpoint::OefAgentEndpoint(std::shared_ptr<ProtoMessageEndpoint<TXType>> endpoint)
  : EndpointPipe(std::move(endpoint))
{
  outstanding_heartbeats = 0;
  count++;
}

void OefAgentEndpoint::close(const std::string &reason)
{
  Counter("mt-core.network.OefAgentEndpoint.closed")++;
  Counter(std::string("mt-core.network.OefAgentEndpoint.closed.") + reason)++;

  socket().close();
}

void OefAgentEndpoint::SetState(const std::string &stateName, bool value)
{
  states[stateName] = value;
}

bool OefAgentEndpoint::GetState(const std::string &stateName) const
{
  auto entry = states.find(stateName);
  if (entry == states.end())
  {
    return false;
  }
  return entry->second;
}

void OefAgentEndpoint::setup(IKarmaPolicy *karmaPolicy)
{
  // can't do this in the constructor because shared_from_this doesn't work in there.
  std::weak_ptr<OefAgentEndpoint> myself_wp = shared_from_this();

  Counter("mt-core.network.OefAgentEndpoint.setup")++;

  endpoint->SetOnStartHandler([myself_wp, karmaPolicy]() {
    if (auto myself_sp = myself_wp.lock())
    {
      Lock lock(myself_sp->mutex);
      Counter("mt-core.network.OefAgentEndpoint.OnStartHandler")++;
      auto k = karmaPolicy->GetAccount(myself_sp->endpoint->GetRemoteId(), "");
      std::swap(k, myself_sp->karma);
      myself_sp->karma.perform("login");
      FETCH_LOG_INFO(LOGGING_NAME, "id=", myself_sp->GetIdentifier(),
                     " KARMA: account=", myself_sp->endpoint->GetRemoteId(),
                     "  balance=", myself_sp->karma.GetBalance());
    }
  });

  auto myGroupId = GetIdentifier();

  endpoint->SetOnCompleteHandler([myGroupId, myself_wp](ConstCharArrayBuffer buffers) {
    if (auto myself_sp = myself_wp.lock())
    {
      Lock lock(myself_sp->mutex);
      Counter("mt-core.network.OefAgentEndpoint.OnCompleteHandler")++;
      myself_sp->karma.perform("message");
      fetch::oef::base::Task::SetThreadGroupId(myGroupId);
      myself_sp->factory->ProcessMessage(buffers);
    }
  });

  endpoint->setOnErrorHandler([myGroupId, myself_wp](std::error_code const &ec) {
    if (auto myself_sp = myself_wp.lock())
    {
      Lock lock(myself_sp->mutex);
      Counter("mt-core.network.OefAgentEndpoint.OnErrorHandler")++;
      myself_sp->karma.perform("error.comms");
      myself_sp->factory->EndpointClosed();
      myself_sp->factory.reset();
      fetch::oef::base::Taskpool::GetDefaultTaskpool().lock()->CancelTaskGroup(myGroupId);
      FETCH_LOG_INFO(LOGGING_NAME, "OnErrorHandler.CancelTaskGroup group=", myGroupId,
                     " , ec=", ec);
    }
  });

  endpoint->SetOnEofHandler([myGroupId, myself_wp]() {
    if (auto myself_sp = myself_wp.lock())
    {
      Lock lock(myself_sp->mutex);
      Counter("mt-core.network.OefAgentEndpoint.OnEofHandler")++;
      myself_sp->karma.perform("eof");
      myself_sp->factory->EndpointClosed();
      myself_sp->factory.reset();
      fetch::oef::base::Taskpool::GetDefaultTaskpool().lock()->CancelTaskGroup(myGroupId);
      FETCH_LOG_INFO(LOGGING_NAME, "OnEofHandler.CancelTaskGroup group=", myGroupId);
    }
  });

  endpoint->SetOnProtoErrorHandler([myGroupId, myself_wp](const std::string &message) {
    if (auto myself_sp = myself_wp.lock())
    {
      Lock lock(myself_sp->mutex);
      Counter("mt-core.network.OefAgentEndpoint.OnProtoErrorHandler")++;
      myself_sp->karma.perform("error.proto");
      myself_sp->factory->EndpointClosed();
      myself_sp->factory.reset();
      fetch::oef::base::Taskpool::GetDefaultTaskpool().lock()->CancelTaskGroup(myGroupId);
      FETCH_LOG_INFO(LOGGING_NAME, "OnProtoErrorHandler.CancelTaskGroup group=", myGroupId,
                     ", message=", message);
    }
  });
}

void OefAgentEndpoint::SetFactory(
    std::shared_ptr<IOefTaskFactory<OefAgentEndpoint>> const &new_factory)
{
  if (factory)
  {
    new_factory->endpoint = factory->endpoint;
  }
  factory = new_factory;
}

OefAgentEndpoint::~OefAgentEndpoint()
{
  endpoint->SetOnCompleteHandler(nullptr);
  endpoint->setOnErrorHandler(nullptr);
  endpoint->SetOnEofHandler(nullptr);
  endpoint->SetOnProtoErrorHandler(nullptr);
  FETCH_LOG_INFO(LOGGING_NAME, "id=", GetIdentifier(), " ~OefAgentEndpoint");
  count--;
}

void OefAgentEndpoint::heartbeat()
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "id=", GetIdentifier(), " HB:", GetIdentifier());
  try
  {
    if (outstanding_heartbeats > 0)
    {
      hb_max_os.max(outstanding_heartbeats);
      FETCH_LOG_DEBUG(LOGGING_NAME, "id=", GetIdentifier(), " HB:", GetIdentifier(),
                      " outstanding=", outstanding_heartbeats);
      karma.perform("comms.outstanding_heartbeats." + std::to_string(outstanding_heartbeats));
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "id=", GetIdentifier(), " HB:", GetIdentifier(), " PING");
    auto ping_message = std::make_shared<fetch::oef::pb::Server_AgentMessage>();
    ping_message->mutable_ping()->set_dummy(1);
    ping_message->set_answer_id(0);
    auto ping_task = std::make_shared<
        TSendProtoTask<OefAgentEndpoint, std::shared_ptr<fetch::oef::pb::Server_AgentMessage>>>(
        ping_message, shared_from_this());

    ping_task->submit();
    hb_sent++;
    outstanding_heartbeats++;
  }
  catch (XKarma const &x)
  {
    socket().close();
  }
}

void OefAgentEndpoint::heartbeat_recvd()
{
  hb_recvd++;
  FETCH_LOG_DEBUG(LOGGING_NAME, "id=", GetIdentifier(), " HB:", GetIdentifier(),
                  " PONG  outstanding=", outstanding_heartbeats, " -> 0");
  outstanding_heartbeats = 0;
}
