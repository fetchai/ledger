#pragma once
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

#include <vector>

#include "network/muddle/muddle_endpoint.hpp"

namespace fetch {
namespace muddle {

/**
 * The fake muddle endpoint can be used for testing to 'send' network traffic to peers within the same process
 */
class FakeMuddleEndpoint : public MuddleEndpoint
{
public:
  // Construction / Destruction
  FakeMuddleEndpoint()          = default;
  ~FakeMuddleEndpoint() = default;

  void Send(Address const &address, uint16_t service, uint16_t channel, Payload const &message) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num, Payload const &payload) override;

  void Broadcast(uint16_t service, uint16_t channel, Payload const &payload) override;
  Response Exchange(Address const &address, uint16_t service, uint16_t channel, Payload const &request) override;

  SubscriptionPtr Subscribe(uint16_t service, uint16_t channel) override;
  SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel) override;

  NetworkId const &network_id() const override;

  AddressList GetDirectlyConnectedPeers() const override;

  //std::shared_ptr<> connections_;
};

inline void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel, Payload const &message)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(message);
}

inline void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num, Payload const &payload)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(message_num);
  FETCH_UNUSED(payload);
}

inline void FakeMuddleEndpoint::Broadcast(uint16_t service, uint16_t channel, Payload const &payload)
{
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(payload);
}

inline FakeMuddleEndpoint::Response FakeMuddleEndpoint::Exchange(Address const &address, uint16_t service, uint16_t channel, Payload const &request)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(request);
  return {};
}

inline FakeMuddleEndpoint::SubscriptionPtr FakeMuddleEndpoint::Subscribe(uint16_t service, uint16_t channel)
{
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  return {};
}

inline FakeMuddleEndpoint::SubscriptionPtr FakeMuddleEndpoint::Subscribe(Address const &address, uint16_t service, uint16_t channel)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  return {};
}

inline NetworkId const &FakeMuddleEndpoint::network_id() const
{
  static NetworkId nid;
  return nid;
}

inline FakeMuddleEndpoint::AddressList FakeMuddleEndpoint::GetDirectlyConnectedPeers() const
{
  return {};
}

}  // namespace muddle
}  // namespace fetch
