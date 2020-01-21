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

#include "core/mutex.hpp"
#include "muddle/muddle_endpoint.hpp"

#include <map>
#include <tuple>
#include <vector>

class FakeMuddleEndpoint : public fetch::muddle::MuddleEndpoint
{
public:
  using Packet    = fetch::muddle::Packet;
  using Address   = fetch::muddle::Address;
  using NetworkId = fetch::muddle::NetworkId;

  FakeMuddleEndpoint(Address address, NetworkId const &network_id);
  ~FakeMuddleEndpoint() override = default;

  /// @name Testing Interface
  /// @{
  void SubmitPacket(Address const &from, uint16_t service, uint16_t channel,
                    Payload const &payload);
  void SubmitPacket(Packet const &packet, Address const &last_hop);
  /// @}

  /// @name Muddle Endpoint Interface
  /// @{
  Address const &GetAddress() const override;
  void           Send(Address const &address, uint16_t service, uint16_t channel,
                      Payload const &message) override;
  void Send(Address const &address, uint16_t service, uint16_t channel, Payload const &message,
            Options options) override;
  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload) override;
  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload, Options options) override;
  void Broadcast(uint16_t service, uint16_t channel, Payload const &payload) override;

  SubscriptionPtr  Subscribe(uint16_t service, uint16_t channel) override;
  SubscriptionPtr  Subscribe(Address const &address, uint16_t service, uint16_t channel) override;
  NetworkId const &network_id() const override;
  AddressList      GetDirectlyConnectedPeers() const override;
  AddressSet       GetDirectlyConnectedPeerSet() const override;
  /// @}

private:
  using ServiceChannel  = std::tuple<uint16_t, uint16_t>;
  using Subscriptions   = std::vector<SubscriptionPtr>;
  using SubscriptionMap = std::map<ServiceChannel, Subscriptions>;

  Address const   address_;
  NetworkId const network_id_;

  fetch::Mutex    lock_;
  SubscriptionMap subscriptions_;
};
