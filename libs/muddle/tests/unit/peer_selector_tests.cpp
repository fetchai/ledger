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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/containers/is_in.hpp"
#include "core/reactor.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "mock_muddle_endpoint.hpp"
#include "muddle/address.hpp"
#include "muddle/network_id.hpp"
#include "muddle_register.hpp"
#include "peer_list.hpp"
#include "peer_selector.hpp"

#include <chrono>
#include <initializer_list>

#include "gtest/gtest.h"

using namespace std::chrono_literals;

using ::testing::NiceMock;
using fetch::core::Reactor;
using fetch::byte_array::FromHex;
using fetch::muddle::PeerSelector;
using fetch::muddle::NetworkId;
using fetch::muddle::MuddleRegister;
using fetch::muddle::PeerConnectionList;
using fetch::muddle::Address;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::serializers::MsgPackSerializer;
using fetch::network::Peer;
using fetch::core::IsIn;

using Peers        = PeerSelector::Peers;
using PeersInfo    = PeerSelector::PeersInfo;
using Metadata     = PeerSelector::Metadata;
using PeerMetadata = PeerSelector::PeerMetadata;

class PeerSelectorTests : public ::testing::Test
{
protected:
  void RunPeerSelector()
  {
    fetch::core::PeriodicRunnable &periodic{peer_selector_};

    periodic.Periodically();
  }

  Address const endpoint_address_{
      FromHex("0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A"
              "2B2C2D2E2F303132333435363738393A3B3C3D3E3F40")};
  NetworkId                    network_id_{"TEST"};
  NiceMock<MockMuddleEndpoint> endpoint_{endpoint_address_, network_id_};
  Reactor                      reactor_{"TEST"};
  MuddleRegister               register_{network_id_};
  PeerConnectionList           peers_{network_id_};
  PeerSelector peer_selector_{network_id_, 1000ms, reactor_, register_, peers_, endpoint_};
};

ConstByteArray CreatePeersPayload(std::initializer_list<std::string> const &addresses)
{
  Peers peers{};
  peers.reserve(addresses.size());

  // populate the peers list
  Peer peer{};
  for (auto const &address : addresses)
  {
    if (peer.Parse(address))
    {
      peers.push_back(peer);
    }
  }

  MsgPackSerializer serialiser{};
  serialiser << peers;
  return serialiser.data();
}

Address CreateAddress(uint64_t index)
{
  ByteArray address{};
  address.Resize(64);

  // clear the address
  memset(address.pointer(), 0, address.size());

  // use index as the address
  address[63] = static_cast<uint8_t>(index & 0xFFu);
  address[62] = static_cast<uint8_t>((index >> 8u) & 0xFFu);
  address[61] = static_cast<uint8_t>((index >> 16u) & 0xFFu);
  address[60] = static_cast<uint8_t>((index >> 24u) & 0xFFu);
  address[59] = static_cast<uint8_t>((index >> 32u) & 0xFFu);
  address[58] = static_cast<uint8_t>((index >> 40u) & 0xFFu);
  address[57] = static_cast<uint8_t>((index >> 48u) & 0xFFu);
  address[56] = static_cast<uint8_t>((index >> 56u) & 0xFFu);

  return {address};
}

TEST_F(PeerSelectorTests, CheckInitialCacheSize)
{
  auto cache = peer_selector_.GetPeerCache();

  EXPECT_TRUE(cache.empty());
}

TEST_F(PeerSelectorTests, BasicAnnouncement)
{
  auto const address1 = CreateAddress(1);
  auto const peers1   = CreatePeersPayload({"127.0.0.1:8000"});

  // emulate recieving of announcement
  endpoint_.fake.SubmitPacket(address1, fetch::SERVICE_MUDDLE, fetch::CHANNEL_ANNOUNCEMENT, peers1);

  PeersInfo cache = peer_selector_.GetPeerCache();
  ASSERT_EQ(1, cache.size());
  ASSERT_TRUE(IsIn(cache, address1));

  Metadata const &metadata = cache[address1];

  ASSERT_EQ(1, metadata.peer_data.size());
  EXPECT_EQ("127.0.0.1:8000", metadata.peer_data[0].peer.ToString());
  EXPECT_FALSE(metadata.peer_data[0].unreachable);
}

TEST_F(PeerSelectorTests, CheckKademliaSelection)
{
  peer_selector_.SetMode(fetch::muddle::PeerSelectionMode::KADEMLIA);

  auto const address1 = CreateAddress(1);
  auto const peers1   = CreatePeersPayload({"127.0.0.1:8000"});

  // emulate recieving of announcement
  endpoint_.fake.SubmitPacket(address1, fetch::SERVICE_MUDDLE, fetch::CHANNEL_ANNOUNCEMENT, peers1);

  PeersInfo cache = peer_selector_.GetPeerCache();
  ASSERT_EQ(1, cache.size());
  ASSERT_TRUE(IsIn(cache, address1));

  Metadata const &metadata = cache[address1];

  ASSERT_EQ(1, metadata.peer_data.size());
  EXPECT_EQ("127.0.0.1:8000", metadata.peer_data[0].peer.ToString());
  EXPECT_FALSE(metadata.peer_data[0].unreachable);

  // run the peer selector
  RunPeerSelector();

  auto const kademlia_peers = peer_selector_.GetKademliaPeers();
  ASSERT_TRUE(IsIn(kademlia_peers, address1));
}

TEST_F(PeerSelectorTests, CheckOverwrite)
{
  peer_selector_.SetMode(fetch::muddle::PeerSelectionMode::KADEMLIA);

  auto const address1 = CreateAddress(1);
  auto const peers1   = CreatePeersPayload({"127.0.0.1:8000"});

  // emulate recieving of announcement
  endpoint_.fake.SubmitPacket(address1, fetch::SERVICE_MUDDLE, fetch::CHANNEL_ANNOUNCEMENT, peers1);

  {
    PeersInfo cache = peer_selector_.GetPeerCache();
    ASSERT_EQ(1, cache.size());
    ASSERT_TRUE(IsIn(cache, address1));

    Metadata const &metadata = cache[address1];

    ASSERT_EQ(1, metadata.peer_data.size());
    EXPECT_EQ("127.0.0.1:8000", metadata.peer_data[0].peer.ToString());
    EXPECT_FALSE(metadata.peer_data[0].unreachable);
  }

  // run the peer selector
  RunPeerSelector();

  auto const kademlia_peers = peer_selector_.GetKademliaPeers();
  ASSERT_TRUE(IsIn(kademlia_peers, address1));

  // the network service is restarted
  auto const address2 = CreateAddress(2);

  // new node makes the announcement
  endpoint_.fake.SubmitPacket(address2, fetch::SERVICE_MUDDLE, fetch::CHANNEL_ANNOUNCEMENT, peers1);

  {
    PeersInfo cache = peer_selector_.GetPeerCache();
    ASSERT_EQ(1, cache.size());
    ASSERT_FALSE(IsIn(cache, address1));
    ASSERT_TRUE(IsIn(cache, address2));

    Metadata const &metadata1 = cache[address1];

    ASSERT_EQ(0, metadata1.peer_data.size());

    Metadata const &metadata2 = cache[address2];

    ASSERT_EQ(1, metadata2.peer_data.size());
    EXPECT_EQ("127.0.0.1:8000", metadata2.peer_data[0].peer.ToString());
    EXPECT_FALSE(metadata2.peer_data[0].unreachable);
  }
}
