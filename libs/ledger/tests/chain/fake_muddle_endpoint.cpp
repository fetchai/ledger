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

#include "muddle/packet.hpp"

#include "fake_muddle_endpoint.hpp"

using fetch::muddle::Subscription;

FakeMuddleEndpoint::FakeMuddleEndpoint(Address address, NetworkId const &network_id)
  : address_{std::move(address)}
  , network_id_{network_id}
{}

void FakeMuddleEndpoint::SubmitPacket(Address const &from, uint16_t service, uint16_t channel,
                                      Payload const &payload)
{
  // build up the muddle packet
  Packet packet{from, network_id_.value()};
  packet.SetService(service);
  packet.SetChannel(channel);
  packet.SetPayload(payload);

  // submit the muddle packet
  SubmitPacket(packet, from);
}

void FakeMuddleEndpoint::SubmitPacket(Packet const &packet, Address const &last_hop)
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

FakeMuddleEndpoint::Address const &FakeMuddleEndpoint::GetAddress() const
{
  return address_;
}

void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                              Payload const &message)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(message);
}

void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                              Payload const &message, Options options)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(message);
  FETCH_UNUSED(options);
}

void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                              uint16_t message_num, Payload const &payload)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(message_num);
  FETCH_UNUSED(payload);
}

void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                              uint16_t message_num, Payload const &payload, Options options)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(message_num);
  FETCH_UNUSED(payload);
  FETCH_UNUSED(options);
}

void FakeMuddleEndpoint::Broadcast(uint16_t service, uint16_t channel, Payload const &payload)
{
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);
  FETCH_UNUSED(payload);
}

FakeMuddleEndpoint::SubscriptionPtr FakeMuddleEndpoint::Subscribe(uint16_t service,
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

FakeMuddleEndpoint::SubscriptionPtr FakeMuddleEndpoint::Subscribe(Address const &address,
                                                                  uint16_t       service,
                                                                  uint16_t       channel)
{
  FETCH_UNUSED(address);
  FETCH_UNUSED(service);
  FETCH_UNUSED(channel);

  throw std::runtime_error{"Exchange not supported in fake muddle currently"};
}

FakeMuddleEndpoint::NetworkId const &FakeMuddleEndpoint::network_id() const
{
  return network_id_;
}

FakeMuddleEndpoint::AddressList FakeMuddleEndpoint::GetDirectlyConnectedPeers() const
{
  return {};
}

FakeMuddleEndpoint::AddressSet FakeMuddleEndpoint::GetDirectlyConnectedPeerSet() const
{
  return {};
}
