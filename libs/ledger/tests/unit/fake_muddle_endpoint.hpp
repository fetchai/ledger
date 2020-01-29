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

using fetch::muddle::Subscription;

inline FakeMuddleEndpoint::FakeMuddleEndpoint(Address address, NetworkId const &network_id)
  : address_{std::move(address)}
  , network_id_{network_id}
{}

inline void FakeMuddleEndpoint::SubmitPacket(Address const &from, uint16_t service,
                                             uint16_t channel, Payload const &payload)
{
  // build up the muddle packet
  Packet packet{from, network_id_.value()};
  packet.SetService(service);
  packet.SetChannel(channel);
  packet.SetPayload(payload);

  // submit the muddle packet
  SubmitPacket(packet, from);
}

inline void FakeMuddleEndpoint::SubmitPacket(Packet const &packet, Address const &last_hop)
{
  Subscriptions subscriptions;

  // lookup the subscriptions if they exist
  {
    FETCH_LOCK(lock_);

    auto it = subscriptions_.find({packet.GetService(), packet.GetChannel()});
    if (it != subscriptions_.end())
    {
      subscriptions = it->second;
    }
  }

  // dispatch the subscriptions
  for (auto const &subscription : subscriptions)
  {
    subscription->Dispatch(packet, last_hop);
  }
}

inline FakeMuddleEndpoint::Address const &FakeMuddleEndpoint::GetAddress() const
{
  return address_;
}

inline void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                                     Payload const &message)
{
  FETCH_UNUSED(address, service, channel, message);
}

inline void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                                     Payload const &message, Options options)
{
  FETCH_UNUSED(address, service, channel, message, options);
}

inline void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                                     uint16_t message_num, Payload const &payload)
{
  FETCH_UNUSED(address, service, channel, message_num, payload);
}

inline void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                                     uint16_t message_num, Payload const &payload, Options options)
{
  FETCH_UNUSED(address, service, channel, message_num, payload, options);
}

inline void FakeMuddleEndpoint::Broadcast(uint16_t service, uint16_t channel,
                                          Payload const &payload)
{
  FETCH_UNUSED(service, channel, payload);
}

inline FakeMuddleEndpoint::SubscriptionPtr FakeMuddleEndpoint::Subscribe(uint16_t service,
                                                                         uint16_t channel)
{
  auto subscription = std::make_shared<Subscription>();

  {
    // update the subscription map
    FETCH_LOCK(lock_);

    auto &subscriptions = subscriptions_[{service, channel}];
    subscriptions.push_back(subscription);
  }

  return subscription;
}

inline FakeMuddleEndpoint::SubscriptionPtr FakeMuddleEndpoint::Subscribe(Address const &address,
                                                                         uint16_t       service,
                                                                         uint16_t       channel)
{
  FETCH_UNUSED(address, service, channel);

  throw std::runtime_error{"Exchange not supported in fake muddle currently"};
}

inline FakeMuddleEndpoint::NetworkId const &FakeMuddleEndpoint::network_id() const
{
  return network_id_;
}

inline FakeMuddleEndpoint::AddressList FakeMuddleEndpoint::GetDirectlyConnectedPeers() const
{
  return {};
}

inline FakeMuddleEndpoint::AddressSet FakeMuddleEndpoint::GetDirectlyConnectedPeerSet() const
{
  return {};
}
