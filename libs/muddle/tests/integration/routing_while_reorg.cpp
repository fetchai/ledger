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
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "network_helpers.hpp"
#include "router.hpp"

#include "gtest/gtest.h"

static constexpr uint16_t SERVICE_ID = 1920;
static constexpr uint16_t CHANNEL_ID = 101;

class MessageCounter
{
public:
  using MuddleInterface = fetch::muddle::MuddleInterface;
  using SubscriptionPtr = fetch::muddle::MuddleEndpoint::SubscriptionPtr;
  using Address         = fetch::muddle::Address;
  using Endpoint        = fetch::muddle::MuddleEndpoint;

  explicit MessageCounter(fetch::muddle::MuddlePtr &muddle)
    : message_endpoint{muddle->GetEndpoint()}
    , message_subscription{message_endpoint.Subscribe(SERVICE_ID, CHANNEL_ID)}
  {
    message_subscription->SetMessageHandler(this, &MessageCounter::NewMessage);
  }

  void NewMessage(fetch::muddle::Packet const &packet, Address const & /*last_hop*/)
  {
    EXPECT_EQ(packet.GetPayload(), "Hello world");
    ++counter;
  }

  Endpoint &            message_endpoint;
  SubscriptionPtr       message_subscription;
  std::atomic<uint64_t> counter{0};
};

TEST(RoutingTests, MessagingWhileReorging)
{
  // Creating network
  std::size_t N       = 10;
  auto        network = Network::New(N, fetch::muddle::TrackerConfiguration::AllOn());
  LinearConnectivity(network, std::chrono::seconds(5));

  auto &reciever = network->nodes[0];

  MessageCounter msgcounter{reciever->muddle};

  for (uint64_t i = 1; i < N; ++i)
  {
    auto &sender = network->nodes[i]->muddle->GetEndpoint();
    sender.Send(reciever->address, SERVICE_ID, CHANNEL_ID, "Hello world");
  }

  // Waiting for all messages to arrive
  int m = 0;
  while (msgcounter.counter != N - 1)
  {
    ++m;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Maximum wait 100s
    if (m > 100)
    {
      break;
    }
  }

  EXPECT_EQ(msgcounter.counter, N - 1);

  network->Stop();
  network->Shutdown();
}
