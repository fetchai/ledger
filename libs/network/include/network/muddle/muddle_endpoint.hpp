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

#include "network/generics/promise_of.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/subscription.hpp"

namespace fetch {
namespace muddle {

/**
 * The muddle endpoint is the abstract interface that is publically exposed between systems and
 * allows users to send and receive packets from the network.
 */
class MuddleEndpoint
{
public:
  using Address         = Packet::Address;
  using Payload         = Packet::Payload;
  using Response        = network::PromiseOf<Payload>;
  using SubscriptionPtr = std::shared_ptr<Subscription>;

  // Construction / Destruction
  MuddleEndpoint()          = default;
  virtual ~MuddleEndpoint() = default;

  /**
   * Send an message to a target address
   *
   * @param address The address (public key) of the target machine
   * @param service The service identifier
   * @param channel The channel identifier
   * @param message The message payload to be sent
   */
  virtual void Send(Address const &address, uint16_t service, uint16_t channel,
                    Payload const &message) = 0;

  /**
   * Send a message to a target address
   *
   * @param address The address (public key) of the target machine
   * @param service The service identifier
   * @param channel The channel identifier
   * @param message_num The message number of the request
   * @param payload The message payload to be sent
   */
  virtual void Send(Address const &address, uint16_t service, uint16_t channel,
                    uint16_t message_num, Payload const &payload) = 0;

  /**
   * Broadcast a message to all peers in the network
   *
   * @param service The service identifier
   * @param channel The channel identifier
   * @param message The message to be sent across the network
   */
  virtual void Broadcast(uint16_t service, uint16_t channel, Payload const &payload) = 0;

  /**
   * Send a request and expect a response back from the target address
   *
   * @param request The request to be sent
   * @param service The service identifier
   * @param channel The channel identifier
   * @return The promise of a response back from the target address
   */
  virtual Response Exchange(Address const &address, uint16_t service, uint16_t channel,
                            Payload const &request) = 0;

  /**
   * Subscribes to messages from network with a given service and channel
   *
   * @param service The identifier for the service
   * @param channel The identifier for the channel
   * @return A valid pointer if the successful, otherwise an invalid pointer
   */
  virtual SubscriptionPtr Subscribe(uint16_t service, uint16_t channel) = 0;

  /**
   * Subscribes to messages from network with a given service and channel
   *
   * @param address The reference to the address
   * @param service The identifier for the service
   * @param channel The identifier for the channel
   * @return A valid pointer if the successful, otherwise an invalid pointer
   */
  virtual SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel) = 0;

  /**
   * Query the network id for this muddle instance
   *
   * @return The network identifier
   */
  virtual uint32_t network_id() const = 0;
};

}  // namespace muddle
}  // namespace fetch
