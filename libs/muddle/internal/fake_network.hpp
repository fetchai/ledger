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

#include "muddle.hpp"

namespace fetch {
namespace muddle {

/**
 * The struct that each address has. Basically a queue of packets
 * with a mutex for access - used by the fake network.
 */
struct PacketQueueAndConnections;

/**
 * Construct a global in-memory network which can be used by fake muddles.
 *
 * To do this, create a map of address to a collection of packets. Peers can then 'send'
 * packets by writing into this collection. If the receiving muddle is listening, it has
 * a responsibility to collect the packets.
 *
 */
class FakeNetwork
{
public:
  using Address                      = Packet::Address;
  using Addresses                    = MuddleInterface::Addresses;
  using PacketQueueAndConnectionsPtr = std::shared_ptr<PacketQueueAndConnections>;
  using FakeNetworkImpl              = std::unordered_map<Address, PacketQueueAndConnectionsPtr>;
  using PacketPtr                    = fetch::muddle::SubscriptionRegistrar::PacketPtr;

  static Addresses GetConnections(Address const &of);
  static void      Register(Address const &of);
  static void      Deregister(Address const &of);
  static void      Connect(Address const &from, Address const &to);
  static void      Disconnect(Address const &from, Address const &to);
  static Addresses GetDirectlyConnectedPeers(Address const &of);
  static void      DeployPacket(Address const &to, PacketPtr packet);
  static void      BroadcastPacket(PacketPtr const &packet);
  static bool      GetNextPacket(Address const &to, PacketPtr &packet);

  // note there are two locks, a global lock on the map of address
  // to the packet struct, and a lock in the packet struct itself.
  // This avoids a bottleneck on the global lock
  static std::mutex      network_lock_;
  static FakeNetworkImpl network_;
};

/**
 * The struct that each address has. Basically a queue of packets
 * with a mutex for access - used by the fake network.
 */
struct PacketQueueAndConnections
{
  using PacketPtr = FakeNetwork::PacketPtr;
  using Address   = FakeNetwork::Address;
  using Addresses = FakeNetwork::Addresses;

  void      Push(PacketPtr packet);
  bool      Pop(PacketPtr &ret);
  Addresses Connections() const;
  void      Connect(Address const &address);

private:
  mutable std::mutex    lock_;
  std::deque<PacketPtr> packets_;
  Addresses             connections_;
};

}  // namespace muddle
}  // namespace fetch
