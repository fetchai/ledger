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

class FakeNetwork
{
public:
  struct PacketQueueAndConnections;

  using Address                      = Packet::Address;  // == a crypto::Identity.identifier_
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

    if (queue)
    {
      queue->Push(std::move(packet));
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

  // Get packet functionality
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

  static std::mutex      network_lock_;
  static FakeNetworkImpl network_;
};

class FakeMuddleEndpoint : public MuddleEndpoint
{
public:
  using Address              = Packet::Address;
  using PacketPtr            = std::shared_ptr<Packet>;
  using Payload              = Packet::Payload;
  using ConnectionPtr        = std::weak_ptr<network::AbstractConnection>;
  using Handle               = network::AbstractConnection::connection_handle_type;
  using ThreadPool           = network::ThreadPool;
  using HandleDirectAddrMap  = std::unordered_map<Handle, Address>;
  using Prover               = crypto::Prover;
  using DirectMessageHandler = std::function<void(Handle, PacketPtr)>;

  // Construction / Destruction
  FakeMuddleEndpoint(NetworkId network_id, Address address, Prover *certificate = nullptr,
                     bool sign_broadcasts = false)
    : network_id_{network_id}
    , address_{address}
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

  ~FakeMuddleEndpoint()
  {
    running_ = false;
    thread_->join();
  }

  Address const &GetAddress() const
  {
    return address_;
  }

  void Send(Address const &address, uint16_t service, uint16_t channel, Payload const &message)
  {
    return Send(address, service, channel, msg_counter_++, message, MuddleEndpoint::OPTION_DEFAULT);
  }

  void Send(Address const &address, uint16_t service, uint16_t channel, Payload const &message,
            Options options)
  {
    return Send(address, service, channel, msg_counter_++, message, options);
  }

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload)
  {
    return Send(address, service, channel, message_num, payload, MuddleEndpoint::OPTION_DEFAULT);
  }

  PacketPtr FormatPacket(Packet::Address const &from, NetworkId const &network, uint16_t service,
                         uint16_t channel, uint16_t counter, uint8_t ttl,
                         Packet::Payload const &payload)
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
    if (certificate_ && (sign_broadcasts_ || !p->IsBroadcast()))
    {
      p->Sign(*certificate_);
    }
    return p;
  }

  void Send(Address const &address, uint16_t service, uint16_t channel, uint16_t message_num,
            Payload const &payload, Options options)
  {
    // format the packet
    auto packet = FormatPacket(address_, network_id_, service, channel, message_num, 40, payload);
    packet->SetTarget(address);

    if (options & OPTION_EXCHANGE)
    {
      packet->SetExchange(true);
    }

    Sign(packet);

    FakeNetwork::DeployPacket(address, packet);
  }

  void Broadcast(uint16_t /*service*/, uint16_t /*channel*/, Payload const & /*payload*/)
  {
    throw 1;
    return;
  }

  Response Exchange(Address const & /*address*/, uint16_t /*service*/, uint16_t /*channel*/,
                    Payload const & /*request*/)
  {
    throw 1;
    return {};
  }

  SubscriptionPtr Subscribe(uint16_t service, uint16_t channel)
  {
    return registrar_.Register(service, channel);
  }

  SubscriptionPtr Subscribe(Address const &address, uint16_t service, uint16_t channel)
  {
    return registrar_.Register(address, service, channel);
  }

  NetworkId const &network_id() const
  {
    return network_id_;
  }

  AddressList GetDirectlyConnectedPeers() const
  {
    auto set_peers = GetDirectlyConnectedPeerSet();

    AddressList ret;

    for (auto const &peer : set_peers)
    {
      ret.push_back(peer);
    }

    return ret;
  }

  AddressSet GetDirectlyConnectedPeerSet() const
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
  using Handle             = network::AbstractConnection::connection_handle_type;
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
    , fake_muddle_endpoint_{network_id_, node_address_, certificate_.get(), sign_broadcasts_}
  {
    FETCH_UNUSED(certificate_);
    FETCH_UNUSED(sign_broadcasts_);
    FETCH_UNUSED(sign_packets_);
  }

  MuddleFake(MuddleFake const &) = delete;
  MuddleFake(MuddleFake &&)      = delete;
  ~MuddleFake()                  = default;

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
    throw 1;
    return {};
  };
  Addresses GetDirectlyConnectedPeers() const override
  {
    return FakeNetwork::GetConnections(node_address_);
  };
  Addresses GetIncomingConnectedPeers() const override
  {
    throw 1;
    return {};
  };
  Addresses GetOutgoingConnectedPeers() const override
  {
    throw 1;
    return {};
  };

  std::size_t GetNumDirectlyConnectedPeers() const override
  {
    return GetDirectlyConnectedPeers().size();
  };
  bool IsDirectlyConnected(Address const & /*address*/) const override
  {
    throw 1;
    return {};
  };
  /// @}

  /// @name Peer Control
  /// @{
  PeerSelectionMode GetPeerSelectionMode() const override
  {
    throw 1;
    return {};
  };
  void SetPeerSelectionMode(PeerSelectionMode /*mode*/) override
  {
    throw 1;
    return;
  };
  Addresses GetRequestedPeers() const override
  {
    throw 1;
    return {};
  };
  void ConnectTo(Address const &address) override
  {
    FakeNetwork::Connect(node_address_, address);
  };
  void ConnectTo(Addresses const & /*addresses*/) override
  {
    throw 1;
    return;
  };
  void ConnectTo(Address const &address, network::Uri const & /*uri_hint*/) override
  {
    FakeNetwork::Connect(node_address_, address);
  };
  void ConnectTo(AddressHints const & /*address_hints*/) override
  {
    throw 1;
    return;
  };
  void DisconnectFrom(Address const & /*address*/) override
  {
    throw 1;
    return;
  };
  void DisconnectFrom(Addresses const & /*addresses*/) override
  {
    throw 1;
    return;
  };
  void SetConfidence(Address const & /*address*/, Confidence /*confidence*/) override
  {
    throw 1;
    return;
  };
  void SetConfidence(Addresses const & /*addresses*/, Confidence /*confidence*/) override
  {
    throw 1;
    return;
  };
  void SetConfidence(ConfidenceMap const & /*map*/) override
  {
    throw 1;
    return;
  };
  /// @}

private:
  using Client          = std::shared_ptr<network::AbstractConnection>;
  using ThreadPool      = network::ThreadPool;
  using Register        = std::shared_ptr<MuddleRegister>;
  using Clock           = std::chrono::system_clock;
  using Timepoint       = Clock::time_point;
  using Duration        = Clock::duration;
  using PeerSelectorPtr = std::shared_ptr<PeerSelector>;

  std::string const    name_;
  char const *const    logging_name_{name_.c_str()};
  CertificatePtr const certificate_;  ///< The private and public keys for the node identity
  std::string const    external_address_;
  Address const        node_address_;
  NetworkManager       network_manager_;  ///< The network manager

  bool sign_packets_;
  bool sign_broadcasts_;

  NetworkId network_id_;

  FakeMuddleEndpoint fake_muddle_endpoint_;
};

}  // namespace muddle
}  // namespace fetch
