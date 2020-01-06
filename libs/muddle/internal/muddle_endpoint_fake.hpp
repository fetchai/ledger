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

#include "fake_network.hpp"

#include "muddle/muddle_endpoint.hpp"

namespace fetch {
namespace muddle {

/**
 * Fake muddle endpoint implements all of the same behaviour as the muddle
 * endpoint, but instead, it has a thread that pulls packets from the global
 * network instance.
 */
class FakeMuddleEndpoint : public MuddleEndpoint
{
public:
  using Address              = Packet::Address;
  using PacketPtr            = std::shared_ptr<Packet>;
  using Payload              = Packet::Payload;
  using ConnectionPtr        = std::weak_ptr<network::AbstractConnection>;
  using Handle               = network::AbstractConnection::ConnectionHandleType;
  using ThreadPool           = network::ThreadPool;
  using HandleDirectAddrMap  = std::unordered_map<Handle, Address>;
  using Prover               = crypto::Prover;
  using DirectMessageHandler = std::function<void(Handle, PacketPtr)>;

  // Construction / Destruction
  FakeMuddleEndpoint(NetworkId network_id, Address address, Prover *certificate = nullptr,
                     bool sign_broadcasts = false);

  ~FakeMuddleEndpoint() override;

  Address const &GetAddress() const override;

  void Send(Address const &address, uint16_t service, uint16_t channel,
            Payload const &message) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, Payload const &message,
            Options options) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload) override;

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload, Options options) override;

  void Broadcast(uint16_t service, uint16_t channel, Payload const &payload) override;

  SubscriptionPtr Subscribe(uint16_t service, uint16_t channel) override;

  SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel) override;

  NetworkId const &network_id() const override;

  AddressList GetDirectlyConnectedPeers() const override;

  AddressSet GetDirectlyConnectedPeerSet() const override;

  PacketPtr const &Sign(PacketPtr const &p) const;

private:
  NetworkId                    network_id_;
  Address                      address_;
  Prover *                     certificate_;
  bool                         sign_broadcasts_;
  SubscriptionRegistrar        registrar_;
  std::atomic<bool>            running_{true};
  std::atomic<uint16_t>        msg_counter_{0};
  std::shared_ptr<std::thread> thread_;
};

}  // namespace muddle
}  // namespace fetch
