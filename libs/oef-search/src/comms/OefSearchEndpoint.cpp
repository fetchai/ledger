//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "oef-search/comms/OefSearchEndpoint.hpp"

#include "oef-base/comms/Endianness.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/monitoring/Gauge.hpp"
#include "oef-base/proto_comms/ProtoMessageReader.hpp"
#include "oef-base/proto_comms/ProtoMessageSender.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-search/comms/SearchTaskFactory.hpp"

static Gauge count("mt-search.network.OefSearchEndpoint");

OefSearchEndpoint::OefSearchEndpoint(std::shared_ptr<ProtoEndpoint> endpoint)
  : Parent(std::move(endpoint))
{
  count++;
  ident = count.get();
}

void OefSearchEndpoint::close(const std::string &reason)
{
  Counter("mt-search.network.OefSearchEndpoint.closed")++;
  Counter(std::string("mt-search.network.OefSearchEndpoint.closed.") + reason)++;

  socket().close();
}

void OefSearchEndpoint::SetState(const std::string &stateName, bool value)
{
  states[stateName] = value;
}

bool OefSearchEndpoint::GetState(const std::string &stateName) const
{
  auto entry = states.find(stateName);
  if (entry == states.end())
  {
    return false;
  }
  return entry->second;
}

void OefSearchEndpoint::setup()
{
  // can't do this in the constructor because shared_from_this doesn't work in there.
  std::weak_ptr<OefSearchEndpoint> myself_wp = shared_from_this();

  endpoint->SetOnStartHandler([myself_wp]() {});

  auto myGroupId = GetIdent();

  endpoint->SetOnCompleteHandler([myGroupId, myself_wp](bool /*success*/, unsigned long /*id*/,
                                                        Uri uri, ConstCharArrayBuffer buffers) {
    if (auto myself_sp = myself_wp.lock())
    {
      Task::SetThreadGroupId(myGroupId);
      myself_sp->factory->current_uri_ = uri;
      myself_sp->factory->ProcessMessage(buffers);
    }
  });

  endpoint->SetOnErrorHandler([myGroupId, myself_wp](std::error_code const & /*ec*/) {
    if (auto myself_sp = myself_wp.lock())
    {
      // myself_sp -> factory -> EndpointClosed();
      // myself_sp -> factory.reset();
      Taskpool::GetDefaultTaskpool().lock()->CancelTaskGroup(myGroupId);
    }
  });

  endpoint->SetOnEofHandler([myGroupId, myself_wp]() {
    if (auto myself_sp = myself_wp.lock())
    {
      // myself_sp -> factory -> EndpointClosed();
      // myself_sp -> factory.reset();
      Taskpool::GetDefaultTaskpool().lock()->CancelTaskGroup(myGroupId);
    }
  });

  endpoint->SetOnProtoErrorHandler([myGroupId, myself_wp](const std::string & /*message*/) {
    if (auto myself_sp = myself_wp.lock())
    {
      // myself_sp -> factory -> EndpointClosed();
      // myself_sp -> factory.reset();
      Taskpool::GetDefaultTaskpool().lock()->CancelTaskGroup(myGroupId);
    }
  });
}

void OefSearchEndpoint::SetFactory(std::shared_ptr<SearchTaskFactory> new_factory)
{
  if (factory)
  {
    new_factory->endpoint = factory->endpoint;
  }
  factory = new_factory;
}

OefSearchEndpoint::~OefSearchEndpoint()
{
  endpoint->SetOnCompleteHandler(nullptr);
  endpoint->SetOnErrorHandler(nullptr);
  endpoint->SetOnEofHandler(nullptr);
  endpoint->SetOnProtoErrorHandler(nullptr);
  FETCH_LOG_INFO(LOGGING_NAME, "~OefSearchEndpoint");
  count--;
}
