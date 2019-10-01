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

#include "muddle.hpp"
#include "muddle/muddle_endpoint.hpp"

#include "muddle_logging_name.hpp"

namespace fetch {
namespace muddle {

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
  struct PacketQueueAndConnections;

  using Address                      = Packet::Address;
  using Addresses                    = MuddleInterface::Addresses;
  using PacketQueueAndConnectionsPtr = std::shared_ptr<PacketQueueAndConnections>;
  using FakeNetworkImpl              = std::unordered_map<Address, PacketQueueAndConnectionsPtr>;
  using PacketPtr                    = fetch::muddle::SubscriptionRegistrar::PacketPtr;

  static Addresses GetConnections(Address const &of)
  {
    std::lock_guard<std::mutex> lock(network_lock_);
    return network_[of]->Connections();
  }

  static void Register(Address const &of)
  {
    std::lock_guard<std::mutex> lock(network_lock_);
    if (!network_[of])
    {
      network_[of] = std::make_shared<PacketQueueAndConnections>();
    }
  }

  static void Deregister(Address const &of)
  {
    std::lock_guard<std::mutex> lock(network_lock_);
    network_.erase(of);
  }

  static void Connect(Address const &from, Address const &to)
  {
    std::lock_guard<std::mutex> lock(network_lock_);
    network_[from]->Connect(to);
    network_[to]->Connect(from);
  }

  static void Disconnect(Address const &from, Address const &to)
  {
    std::lock_guard<std::mutex> lock(network_lock_);

    if (network_.find(from) != network_.end())
    {
      network_[from]->Connect(to);
    }

    if (network_.find(to) != network_.end())
    {
      network_[to]->Connect(from);
    }
  }

  static Addresses GetDirectlyConnectedPeers(Address const &of)
  {
    std::lock_guard<std::mutex> lock(network_lock_);
    return network_[of]->Connections();
  }

  static void DeployPacket(Address const &to, PacketPtr packet)
  {
    PacketQueueAndConnectionsPtr queue;

    {
      std::lock_guard<std::mutex> lock(network_lock_);
      if (network_.find(to) != network_.end())
      {
        queue = network_[to];
      }
    }

    // Note access without global lock for performance
    if (queue)
    {
      queue->Push(std::move(packet));
    }
  }

  static void BroadcastPacket(PacketPtr const &packet)
  {
    std::vector<PacketQueueAndConnectionsPtr> locations;

    {
      std::lock_guard<std::mutex> lock(network_lock_);
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

  static bool GetNextPacket(Address const &to, PacketPtr &packet)
  {
    PacketQueueAndConnectionsPtr queue;

    {
      std::lock_guard<std::mutex> lock(network_lock_);
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

  /**
   * The struct that each address has. Basically a queue of packets
   * with a mutex for access
   */
  struct PacketQueueAndConnections
  {
    void Push(PacketPtr packet)
    {
      std::lock_guard<std::mutex> lock(lock_);
      packets_.push_front(std::move(packet));
    }

    bool Pop(PacketPtr &ret)
    {
      std::lock_guard<std::mutex> lock(lock_);

      if (!packets_.empty())
      {
        ret = std::move(packets_.back());
        packets_.pop_back();
        return true;
      }

      return false;
    }

    Addresses Connections()
    {
      std::lock_guard<std::mutex> lock(lock_);
      return connections_;
    }

    void Connect(Address const &address)
    {
      std::lock_guard<std::mutex> lock(lock_);
      connections_.insert(address);
    }

    std::mutex            lock_;
    std::deque<PacketPtr> packets_;
    Addresses             connections_;
  };

  // note there are two locks, a global lock on the map of address
  // to the packet struct, and a lock in the packet struct itself.
  // This avoids a bottleneck on the global lock
  static std::mutex      network_lock_;
  static FakeNetworkImpl network_;
};

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
                     bool sign_broadcasts = false)
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

  ~FakeMuddleEndpoint() override
  {
    running_ = false;
    thread_->join();
  }

  Address const &GetAddress() const override
  {
    return address_;
  }

  void Send(Address const &address, uint16_t service, uint16_t channel,
            Payload const &message) override
  {
    return Send(address, service, channel, msg_counter_++, message, MuddleEndpoint::OPTION_DEFAULT);
  }

  void Send(Address const &address, uint16_t service, uint16_t channel, Payload const &message,
            Options options) override
  {
    return Send(address, service, channel, msg_counter_++, message, options);
  }

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload) override
  {
    return Send(address, service, channel, message_num, payload, MuddleEndpoint::OPTION_DEFAULT);
  }

  PacketPtr FormatPacket(Packet::Address const &from, NetworkId const &network, uint16_t service,
                         uint16_t channel, uint16_t counter, uint8_t ttl,
                         Packet::Payload const &payload) override
  {
    auto packet = std::make_shared<Packet>(from, network.value());
    packet->SetService(service);
    packet->SetChannel(channel);
    packet->SetMessageNum(counter);
    packet->SetTTL(ttl);
    packet->SetPayload(payload);

    return packet;
  }

  FakeMuddleEndpoint::PacketPtr const &Sign(PacketPtr const &p) const
  {
    if (certificate_ && (sign_broadcasts_ || !p->IsBroadcast()))  // NOLINT
    {
      p->Sign(*certificate_);
    }
    return p;
  }

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload, Options options) override
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

  void Broadcast(uint16_t service, uint16_t channel, Payload const &payload) override
  {
    auto packet =
        FormatPacket(address_, network_id_, service, channel, msg_counter_++, 40, payload);
    packet->SetBroadcast(true);
    Sign(packet);

    FakeNetwork::BroadcastPacket(packet);
  }

  Response Exchange(Address const & /*address*/, uint16_t /*service*/, uint16_t /*channel*/,
                    Payload const & /*request*/) override
  {
    throw std::runtime_error("Exchange functionality not implemented");
    return {};
  }

  SubscriptionPtr Subscribe(uint16_t service, uint16_t channel) override
  {
    return registrar_.Register(service, channel);
  }

  SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel)
  {
    return registrar_.Register(address, service, channel);
  }

  NetworkId const &network_id() const override
  {
    return network_id_;
  }

  AddressList GetDirectlyConnectedPeers() const override
  {
    auto set_peers = GetDirectlyConnectedPeerSet();

    AddressList ret;

    for (auto const &peer : set_peers)
    {
      ret.push_back(peer);
    }

    return ret;
  }

  AddressSet GetDirectlyConnectedPeerSet() const override
  {
    return FakeNetwork::GetDirectlyConnectedPeers(address_);
  }

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

/**
 * Fake muddle just tells the global network which connections to make
 */
class MuddleFake final : public MuddleInterface, public std::enable_shared_from_this<MuddleFake>
{
public:
  using CertificatePtr     = std::shared_ptr<crypto::Prover>;
  using Uri                = network::Uri;
  using UriList            = std::vector<Uri>;
  using NetworkManager     = network::NetworkManager;
  using FakeMuddleEndpoint = muddle::FakeMuddleEndpoint;
  using Promise            = service::Promise;
  using Identity           = crypto::Identity;
  using Address            = Router::Address;
  using ConnectionState    = PeerConnectionList::ConnectionState;
  using Handle             = network::AbstractConnection::ConnectionHandleType;
  using Server             = std::shared_ptr<network::AbstractNetworkServer>;
  using ServerList         = std::vector<Server>;

  // Construction / Destruction
  MuddleFake(NetworkId network_id, CertificatePtr certificate, NetworkManager const &nm,
             bool sign_packets = false, bool sign_broadcasts = false,
             std::string external_address = "127.0.0.1")
    : name_{GenerateLoggingName("Muddle", network_id)}
    , certificate_(std::move(certificate))
    , external_address_(std::move(external_address))
    , node_address_(certificate_->identity().identifier())
    , network_manager_(nm)
    , sign_packets_{sign_packets}
    , sign_broadcasts_{sign_broadcasts}
    , network_id_{network_id}
    , fake_muddle_endpoint_{network_id_, node_address_, sign_packets ? certificate_.get() : nullptr,
                            sign_packets && sign_broadcasts}
  {
    FETCH_UNUSED(certificate_);
    FETCH_UNUSED(sign_broadcasts_);
    FETCH_UNUSED(sign_packets_);
  }

  MuddleFake(MuddleFake const &) = delete;
  MuddleFake(MuddleFake &&)      = delete;
  ~MuddleFake() override         = default;

  /// @name Muddle Setup
  /// @{
  bool Start(Peers const & /*peers*/, Ports const & /*ports*/) override
  {
    FakeNetwork::Register(node_address_);
    return true;
  };
  bool Start(Uris const & /*peers*/, Ports const & /*ports*/) override
  {
    FakeNetwork::Register(node_address_);
    return true;
  };
  bool Start(Ports const & /*ports*/) override
  {
    FakeNetwork::Register(node_address_);
    return true;
  };
  void Stop() override
  {
    FakeNetwork::Deregister(node_address_);
  };
  MuddleEndpoint &GetEndpoint() override
  {
    return fake_muddle_endpoint_;
  };
  /// @}

  /// @name Muddle Status
  /// @{
  NetworkId const &GetNetwork() const override
  {
    return network_id_;
  };
  Address const &GetAddress() const override
  {
    return node_address_;
  };
  Ports GetListeningPorts() const override
  {
    throw std::runtime_error("Get listening ports functionality not implemented");
    return {};
  };
  Addresses GetDirectlyConnectedPeers() const override
  {
    return FakeNetwork::GetConnections(node_address_);
  };
  Addresses GetIncomingConnectedPeers() const override
  {
    throw std::runtime_error("GetIncomingConnectedPeers functionality not implemented");
    return {};
  };
  Addresses GetOutgoingConnectedPeers() const override
  {
    throw std::runtime_error("GetOutgoingConnectedPeers functionality not implemented");
    return {};
  };

  std::size_t GetNumDirectlyConnectedPeers() const override
  {
    return GetDirectlyConnectedPeers().size();
  };
  bool IsDirectlyConnected(Address const & /*address*/) const override
  {
    throw std::runtime_error("IsDirectlyConnected functionality not implemented");
    return {};
  };
  /// @}

  /// @name Peer Control
  /// @{
  PeerSelectionMode GetPeerSelectionMode() const override
  {
    throw std::runtime_error("GetPeerSelectionMode functionality not implemented");
    return {};
  };
  void SetPeerSelectionMode(PeerSelectionMode /*mode*/) override
  {
    throw std::runtime_error("SetPeerSelectionMode functionality not implemented");
  };
  Addresses GetRequestedPeers() const override
  {
    return FakeNetwork::GetConnections(node_address_);
  };
  void ConnectTo(Address const &address) override
  {
    FakeNetwork::Connect(node_address_, address);
  };
  void ConnectTo(Addresses const & /*addresses*/) override
  {
    throw std::runtime_error("ConnectTo x functionality not implemented");
  };
  void ConnectTo(Address const &address, network::Uri const & /*uri_hint*/) override
  {
    FakeNetwork::Connect(node_address_, address);
  };
  void ConnectTo(AddressHints const & /*address_hints*/) override
  {
    throw std::runtime_error("ConnectTo y functionality not implemented");
  };
  void DisconnectFrom(Address const &address) override
  {
    FakeNetwork::Disconnect(node_address_, address);
  };
  void DisconnectFrom(Addresses const &addresses) override
  {
    for (auto const &address : addresses)
    {
      FakeNetwork::Disconnect(node_address_, address);
    }
  };
  void SetConfidence(Address const & /*address*/, Confidence /*confidence*/) override
  {
    throw std::runtime_error("SetConfidence x functionality not implemented");
  };
  void SetConfidence(Addresses const & /*addresses*/, Confidence /*confidence*/) override
  {
    throw std::runtime_error("SetConfidence y functionality not implemented");
  };
  void SetConfidence(ConfidenceMap const & /*map*/) override
  {
    throw std::runtime_error("SetConfidence z functionality not implemented");
  };
  /// @}

private:
  std::string const    name_;
  char const *const    logging_name_{name_.c_str()};
  CertificatePtr const certificate_;  ///< The private and public keys for the node identity
  std::string const    external_address_;
  Address const        node_address_;
  NetworkManager       network_manager_;  ///< The network manager

  bool sign_packets_;
  bool sign_broadcasts_;

  NetworkId          network_id_;
  FakeMuddleEndpoint fake_muddle_endpoint_;
};

}  // namespace muddle
}  // namespace fetch
