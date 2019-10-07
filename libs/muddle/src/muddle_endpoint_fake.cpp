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

#include "muddle_endpoint_fake.hpp"

using fetch::muddle::FakeMuddleEndpoint;
using fetch::muddle::NetworkId;

using Packet          = fetch::muddle::Packet;
using PacketPtr       = FakeMuddleEndpoint::PacketPtr;
using Address         = FakeMuddleEndpoint::Address;
using Response        = FakeMuddleEndpoint::Response;
using SubscriptionPtr = FakeMuddleEndpoint::SubscriptionPtr;
using AddressList     = FakeMuddleEndpoint::AddressList;
using AddressSet      = FakeMuddleEndpoint::AddressSet;

// Convenience function - note shares similarities with real muddle
PacketPtr FormatPacket(Packet::Address from, NetworkId const &network, uint16_t service,
                       uint16_t channel, uint16_t counter, uint8_t ttl,
                       const Packet::Payload &payload)
{
  auto packet = std::make_shared<Packet>(std::move(from), network.value());
  packet->SetService(service);
  packet->SetChannel(channel);
  packet->SetMessageNum(counter);
  packet->SetTTL(ttl);
  packet->SetPayload(payload);

  return packet;
}

PacketPtr const &FakeMuddleEndpoint::Sign(PacketPtr const &p) const
{
  if ((certificate_ != nullptr) && (sign_broadcasts_ || !p->IsBroadcast()))
  {
    p->Sign(*certificate_);
  }
  return p;
}

// Construction / Destruction
FakeMuddleEndpoint::FakeMuddleEndpoint(NetworkId network_id, Address address, Prover *certificate,
                                       bool sign_broadcasts)
  : network_id_{network_id}
  , address_{std::move(address)}
  , certificate_{certificate}
  , sign_broadcasts_{sign_broadcasts}
  , registrar_(network_id)
{
  FETCH_UNUSED(certificate_);
  FETCH_UNUSED(sign_broadcasts_);

  thread_ = std::make_shared<std::thread>([this]() {
    PacketPtr next_packet;

    while (running_)
    {
      if (FakeNetwork::GetNextPacket(address_, next_packet))
      {
        registrar_.Dispatch(next_packet, next_packet->GetSender());
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  });
}

FakeMuddleEndpoint::~FakeMuddleEndpoint()
{
  running_ = false;
  thread_->join();
}

Address const &FakeMuddleEndpoint::GetAddress() const
{
  return address_;
}

void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                              Payload const &message)
{
  return Send(address, service, channel, msg_counter_++, message, MuddleEndpoint::OPTION_DEFAULT);
}

void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                              Payload const &message, Options options)
{
  return Send(address, service, channel, msg_counter_++, message, options);
}

void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                              uint16_t message_num, Payload const &payload)
{
  return Send(address, service, channel, message_num, payload, MuddleEndpoint::OPTION_DEFAULT);
}

void FakeMuddleEndpoint::Send(Address const &address, uint16_t service, uint16_t channel,
                              uint16_t message_num, Payload const &payload, Options options)
{
  // format the packet
  auto packet = FormatPacket(address_, network_id_, service, channel, message_num, 40, payload);
  packet->SetTarget(address);

  if (bool(options & OPTION_EXCHANGE))
  {
    packet->SetExchange(true);
  }

  Sign(packet);

  FakeNetwork::DeployPacket(address, packet);
}

void FakeMuddleEndpoint::Broadcast(uint16_t service, uint16_t channel, Payload const &payload)
{
  auto packet = FormatPacket(address_, network_id_, service, channel, msg_counter_++, 40, payload);
  packet->SetBroadcast(true);
  Sign(packet);

  FakeNetwork::BroadcastPacket(packet);
}

Response FakeMuddleEndpoint::Exchange(Address const & /*address*/, uint16_t /*service*/,
                                      uint16_t /*channel*/, Payload const & /*request*/)
{
  throw std::runtime_error("Exchange functionality not implemented");
  return {};
}

SubscriptionPtr FakeMuddleEndpoint::Subscribe(uint16_t service, uint16_t channel)
{
  return registrar_.Register(service, channel);
}

SubscriptionPtr FakeMuddleEndpoint::Subscribe(Address const &address, uint16_t service,
                                              uint16_t channel)
{
  return registrar_.Register(address, service, channel);
}

NetworkId const &FakeMuddleEndpoint::network_id() const
{
  return network_id_;
}

AddressList FakeMuddleEndpoint::GetDirectlyConnectedPeers() const
{
  auto set_peers = GetDirectlyConnectedPeerSet();

  AddressList ret;

  for (auto const &peer : set_peers)
  {
    ret.push_back(peer);
  }

  return ret;
}

AddressSet FakeMuddleEndpoint::GetDirectlyConnectedPeerSet() const
{
  return FakeNetwork::GetDirectlyConnectedPeers(address_);
}
