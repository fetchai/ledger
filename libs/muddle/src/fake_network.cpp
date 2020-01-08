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

using fetch::muddle::FakeNetwork;
using Addresses = fetch::muddle::FakeNetwork::Addresses;

// Statics required for fake network
std::mutex                   FakeNetwork::network_lock_{};
FakeNetwork::FakeNetworkImpl FakeNetwork::network_{};

Addresses FakeNetwork::GetConnections(Address const &of)
{
  FETCH_LOCK(network_lock_);
  return network_[of]->Connections();
}

void FakeNetwork::Register(Address const &of)
{
  FETCH_LOCK(network_lock_);
  if (!network_[of])
  {
    network_[of] = std::make_shared<PacketQueueAndConnections>();
  }
}

void FakeNetwork::Deregister(Address const &of)
{
  FETCH_LOCK(network_lock_);
  network_.erase(of);
}

void FakeNetwork::Connect(Address const &from, Address const &to)
{
  FETCH_LOCK(network_lock_);
  network_[from]->Connect(to);
  network_[to]->Connect(from);
}

void FakeNetwork::Disconnect(Address const &from, Address const &to)
{
  FETCH_LOCK(network_lock_);

  if (network_.find(from) != network_.end())
  {
    network_[from]->Connect(to);
  }

  if (network_.find(to) != network_.end())
  {
    network_[to]->Connect(from);
  }
}

Addresses FakeNetwork::GetDirectlyConnectedPeers(Address const &of)
{
  FETCH_LOCK(network_lock_);
  return network_[of]->Connections();
}

void FakeNetwork::DeployPacket(Address const &to, PacketPtr packet)
{
  PacketQueueAndConnectionsPtr queue;

  {
    FETCH_LOCK(network_lock_);

    auto it = network_.find(to);

    if (it != network_.end())
    {
      queue = it->second;
    }
  }

  // Note access without global lock for performance
  if (queue)
  {
    queue->Push(std::move(packet));
  }
}

void FakeNetwork::BroadcastPacket(PacketPtr const &packet)
{
  std::vector<PacketQueueAndConnectionsPtr> locations;

  {
    FETCH_LOCK(network_lock_);

    locations.reserve(network_.size());

    for (auto const &network_location : network_)
    {
      locations.push_back(network_location.second);
    }
  }

  for (auto const &location : locations)
  {
    location->Push(packet);
  }
}

bool FakeNetwork::GetNextPacket(Address const &to, PacketPtr &packet)
{
  PacketQueueAndConnectionsPtr queue;

  {
    FETCH_LOCK(network_lock_);
    if (network_.find(to) != network_.end())
    {
      queue = network_[to];
    }
  }

  if (queue)
  {
    return (queue->Pop(packet));
  }

  return false;
}

// Methods for the packet holding struct
using fetch::muddle::PacketQueueAndConnections;

void PacketQueueAndConnections::Push(PacketPtr packet)
{
  FETCH_LOCK(lock_);
  packets_.push_front(std::move(packet));
}

bool PacketQueueAndConnections::Pop(PacketPtr &ret)
{
  FETCH_LOCK(lock_);

  if (!packets_.empty())
  {
    ret = std::move(packets_.back());
    packets_.pop_back();
    return true;
  }

  return false;
}

Addresses PacketQueueAndConnections::Connections() const
{
  FETCH_LOCK(lock_);
  return connections_;
}

void PacketQueueAndConnections::Connect(Address const &address)
{
  FETCH_LOCK(lock_);
  connections_.insert(address);
}
