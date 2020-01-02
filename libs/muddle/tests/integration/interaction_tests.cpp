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

#include "core/synchronisation/protected.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "network/management/network_manager.hpp"

#include "gtest/gtest.h"

#include <chrono>
#include <sstream>
#include <thread>

namespace {

using fetch::network::NetworkManager;
using fetch::muddle::CreateMuddle;
using fetch::muddle::Packet;
using fetch::muddle::MuddlePtr;
using fetch::muddle::MuddleEndpoint;
using fetch::network::Uri;
using fetch::network::Peer;
using std::this_thread::sleep_for;

using namespace std::chrono_literals;

constexpr char const *LOGGING_NAME = "InteractionTests";

class InteractionTests : public ::testing::Test
{
protected:
  using SubscriptionPtr  = MuddleEndpoint::SubscriptionPtr;
  using SubscriptionPtrs = std::vector<SubscriptionPtr>;
  using MuddleAddress    = fetch::muddle::Address;
  using Counters         = std::unordered_map<uint64_t, std::unordered_map<uint64_t, uint64_t>>;
  using SyncCounters     = fetch::Protected<Counters>;

  static constexpr uint16_t SERVICE = 1;
  static constexpr uint16_t CHANNEL = 2;

  void SetUp() override
  {
    nm_.Start();

    // create the first muddle node
    node1_ = CreateMuddle("Test", nm_, "127.0.0.1");
    node1_->Start({0});

    node2_ = CreateMuddle("Test", nm_, "127.0.0.1");
    node2_->Start({0});

    node3_ = CreateMuddle("Test", nm_, "127.0.0.1");
    node3_->Start({0});

    // fully connect the nodes
    node3_->ConnectTo(node1_->GetAddress(), GetUriToMuddle(node1_));
    node1_->ConnectTo(node2_->GetAddress(), GetUriToMuddle(node2_));
    node2_->ConnectTo(node1_->GetAddress(), GetUriToMuddle(node1_));
    node1_->ConnectTo(node3_->GetAddress(), GetUriToMuddle(node3_));
    node2_->ConnectTo(node3_->GetAddress(), GetUriToMuddle(node3_));
    node3_->ConnectTo(node2_->GetAddress(), GetUriToMuddle(node2_));

    // wait for all the peers to connect
    for (auto const &node : {node1_, node2_, node3_})
    {
      subscriptions_.emplace_back(node->GetEndpoint().Subscribe(SERVICE, CHANNEL));
      subscriptions_.back()->SetMessageHandler(this, &InteractionTests::OnMessage);

      while (node->GetNumDirectlyConnectedPeers() != 2)
      {
        sleep_for(1000ms);
      }

      auto const connected_peers = node->GetDirectlyConnectedPeers();
      FETCH_LOG_INFO(LOGGING_NAME, "Node: ", NodeIndex(node->GetAddress()));
      for (auto const &peer : connected_peers)
      {
        FETCH_LOG_INFO(LOGGING_NAME, " - Connected to: ", NodeIndex(peer));
      }
    }

    sleep_for(1000ms);
  }

  void TearDown() override
  {
    for (auto const &node : {node1_, node2_, node3_})
    {
      node->Stop();
    }

    nm_.Stop();
  }

  uint64_t NodeIndex(MuddleAddress const &address) const
  {
    uint64_t index{0};

    if (address == node1_->GetAddress())
    {
      index = 1;
    }
    else if (address == node2_->GetAddress())
    {
      index = 2;
    }
    else if (address == node3_->GetAddress())
    {
      index = 3;
    }

    return index;
  }

  void OnMessage(Packet const &packet, MuddleAddress const & /*address*/)
  {
    counters_.ApplyVoid([this, &packet](Counters &counters) {
      ++(counters[NodeIndex(packet.GetSender())][NodeIndex(packet.GetTarget())]);
    });
  }

  static uint16_t GetListeningPort(MuddlePtr const &muddle)
  {
    uint16_t port{0};
    for (;;)
    {
      auto const ports = muddle->GetListeningPorts();
      if (!ports.empty())
      {
        for (auto it = ports.begin(), end = ports.end(); (it != end) && (port == 0); ++it)
        {
          // update the port
          port = *it;
        }

        // exit the outer loop when we have a non-zero port
        if (port != 0u)
        {
          break;
        }
      }

      sleep_for(500ms);
    }

    return port;
  }

  static Uri GetUriToMuddle(MuddlePtr const &muddle)
  {
    return Uri{Peer{"127.0.0.1", GetListeningPort(muddle)}};
  }

  NetworkManager   nm_{"test", 3};
  MuddlePtr        node1_;
  MuddlePtr        node2_;
  MuddlePtr        node3_;
  SubscriptionPtrs subscriptions_;

  SyncCounters counters_;
};

TEST_F(InteractionTests, DISABLED_MutualConnections)
{
  static const std::size_t NUM_MESSAGES     = 1000;
  static const std::size_t MAX_NUM_ERRORS   = 1;
  static const std::size_t MIN_NUM_MESSAGES = NUM_MESSAGES - MAX_NUM_ERRORS;

  for (std::size_t i = 0; i < NUM_MESSAGES; ++i)
  {
    node1_->GetEndpoint().Send(node2_->GetAddress(), SERVICE, CHANNEL, "hello");
    node1_->GetEndpoint().Send(node3_->GetAddress(), SERVICE, CHANNEL, "hello");

    node2_->GetEndpoint().Send(node1_->GetAddress(), SERVICE, CHANNEL, "hello");
    node2_->GetEndpoint().Send(node3_->GetAddress(), SERVICE, CHANNEL, "hello");

    node3_->GetEndpoint().Send(node1_->GetAddress(), SERVICE, CHANNEL, "hello");
    node3_->GetEndpoint().Send(node2_->GetAddress(), SERVICE, CHANNEL, "hello");
  }

  counters_.ApplyVoid([](Counters const &counters) {

#ifdef FETCH_LOG_DEBUG_ENABLED
    std::ostringstream oss;
    oss << "\n\n";

    for (auto const &element : counters)
    {
      for (auto const &other : element.second)
      {
        oss << element.first << " -> " << other.first << " : " << other.second << '\n';
      }
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, oss.str());
#endif

    for (auto const &element : counters)
    {
      for (auto const &other : element.second)
      {
        EXPECT_GE(other.second, MIN_NUM_MESSAGES);
        EXPECT_LE(other.second, NUM_MESSAGES);
      }
    }
  });
}

}  // namespace
