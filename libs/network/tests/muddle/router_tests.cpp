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

#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "network/management/network_manager.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/router.hpp"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

namespace {

using NetworkManager    = fetch::network::NetworkManager;
using NetworkManagerPtr = std::unique_ptr<NetworkManager>;
using Uri               = fetch::network::Uri;
using Muddle            = fetch::muddle::Muddle;
using Payload           = fetch::muddle::Packet::Payload;
using Address           = Muddle::Address;
using MuddlePtr         = std::unique_ptr<Muddle>;
using Certificate       = fetch::crypto::Prover;
using CertificatePtr    = std::unique_ptr<Certificate>;

using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using fetch::muddle::NetworkId;

struct Message
{
  Address  from;
  uint16_t service;
  uint16_t channel;
  uint16_t counter;
  Payload  payload;
};

struct MessageQueue
{
  using Queue     = std::vector<Message>;
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;

  mutable std::mutex lock;
  Queue              messages;

  template <typename CB>
  void Visit(CB const &callback)
  {
    std::lock_guard<std::mutex> guard(lock);
    callback(messages);
  }

  void Add(Message const &payload)
  {
    std::lock_guard<std::mutex> guard(lock);
    messages.push_back(payload);
  }

  template <typename D, typename R>
  bool Wait(std::size_t message_count, std::chrono::duration<D, R> const &duration)
  {
    bool success = false;

    Timepoint const deadline = Clock::now() + duration;

    while (Clock::now() < deadline)
    {
      // determine if the message count has been reached
      {
        std::lock_guard<std::mutex> guard(lock);
        if (messages.size() >= message_count)
        {
          success = true;
          break;
        }
      }

      sleep_for(milliseconds{100});
    }

    return success;
  }
};

class RouterTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    network_manager_ = std::make_unique<NetworkManager>("NetMgr", 6);
    network_manager_->Start();
  }

  void TearDown() override
  {
    network_manager_->Stop();
    network_manager_.reset();
  }

  MuddlePtr CreateMuddle()
  {
    // generate the identity
    auto identity = std::make_unique<fetch::crypto::ECDSASigner>();
    identity->GenerateKeys();

    return std::make_unique<Muddle>(NetworkId{"Test"}, std::move(identity),
                                    *network_manager_);
  }

  NetworkManagerPtr network_manager_;
};

TEST_F(RouterTests, CheckExchange)
{
  static constexpr uint16_t SERVICE = 1;
  static constexpr uint16_t CHANNEL = 2;

  auto  nodeA     = CreateMuddle();
  auto &endpointA = nodeA->AsEndpoint();
  nodeA->Start({8000});

  auto  nodeB     = CreateMuddle();
  auto &endpointB = nodeB->AsEndpoint();
  nodeB->Start({8001}, {Uri{"tcp://127.0.0.1:8000"}});

  // define the message queues
  MessageQueue messagesA;
  MessageQueue messagesB;

  // register the receive for node A
  auto subA = endpointA.Subscribe(SERVICE, CHANNEL);
  subA->SetMessageHandler([&messagesA](Address const &from, uint16_t service, uint16_t channel,
                                       uint16_t counter, Payload const &payload, Address const &) {
    messagesA.Add(Message{from, service, channel, counter, payload});
  });

  // register the receive for node B
  auto subB = endpointB.Subscribe(SERVICE, CHANNEL);
  subB->SetMessageHandler([&messagesB](Address const &from, uint16_t service, uint16_t channel,
                                       uint16_t counter, Payload const &payload, Address const &) {
    messagesB.Add(Message{from, service, channel, counter, payload});
  });

  // wait for the two nodes to connect
  sleep_for(milliseconds{750});

  // get A to broadcast a series of messages
  endpointA.Broadcast(SERVICE, CHANNEL, "Node A Message 1");
  endpointA.Broadcast(SERVICE, CHANNEL, "Node A Message 2");
  endpointA.Broadcast(SERVICE, CHANNEL, "Node A Message 3");

  // allow enough time for the messages to be sent
  ASSERT_TRUE(messagesB.Wait(3, milliseconds{3000}));

  // check the contents of the messages
  messagesB.Visit([&nodeA](MessageQueue::Queue &messages) {
    ASSERT_EQ(messages.size(), 3);

    // check the contents of the messages
    EXPECT_EQ(messages[0].from, nodeA->identity().identifier());
    EXPECT_EQ(messages[0].service, SERVICE);
    EXPECT_EQ(messages[0].channel, CHANNEL);
    EXPECT_EQ(messages[0].counter, 1);
    EXPECT_EQ(messages[0].payload, "Node A Message 1");

    // check the contents of the messages
    EXPECT_EQ(messages[1].from, nodeA->identity().identifier());
    EXPECT_EQ(messages[1].service, SERVICE);
    EXPECT_EQ(messages[1].channel, CHANNEL);
    EXPECT_EQ(messages[1].counter, 2);
    EXPECT_EQ(messages[1].payload, "Node A Message 2");

    // check the contents of the messages
    EXPECT_EQ(messages[2].from, nodeA->identity().identifier());
    EXPECT_EQ(messages[2].service, SERVICE);
    EXPECT_EQ(messages[2].channel, CHANNEL);
    EXPECT_EQ(messages[2].counter, 3);
    EXPECT_EQ(messages[2].payload, "Node A Message 3");

    // tidy up
    messages.clear();
  });

  // get B to broadcast a response
  endpointB.Broadcast(SERVICE, CHANNEL, "Node B Message 1");

  // allow enough time for the messages to be sent
  ASSERT_TRUE(messagesA.Wait(1, milliseconds{1500}));

  nodeB->Stop();
  nodeA->Stop();
}

}  // namespace
