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

#include "network/muddle/dispatcher.hpp"

#include "gmock/gmock.h"

#include <memory>

class DispatcherTests : public ::testing::Test
{
protected:
  using Packet        = fetch::muddle::Packet;
  using PacketPtr     = std::shared_ptr<Packet>;
  using Payload       = Packet::Payload;
  using Dispatcher    = fetch::muddle::Dispatcher;
  using Promise       = Dispatcher::Promise;
  using DispatcherPtr = std::unique_ptr<Dispatcher>;

  void SetUp() override
  {
    dispatcher_ = std::make_unique<Dispatcher>();
  }

  PacketPtr CreatePacket(uint16_t service, uint16_t channel, uint16_t counter,
                         Payload const & /*payload*/)
  {
    PacketPtr packet = std::make_shared<Packet>();
    packet->SetService(service);
    packet->SetProtocol(channel);
    packet->SetMessageNum(counter);

    return packet;
  }

  DispatcherPtr dispatcher_;
};

TEST_F(DispatcherTests, CheckExchange)
{
  Payload   response("hello");
  PacketPtr packet = CreatePacket(1, 2, 3, response);

  // register the exchange
  Promise prom = dispatcher_->RegisterExchange(1, 2, 3, packet->GetSender());

  dispatcher_->Dispatch(packet);

  EXPECT_FALSE(prom->IsWaiting());
  EXPECT_FALSE(prom->IsFailed());
  EXPECT_TRUE(prom->IsSuccessful());
  EXPECT_TRUE(prom->Wait(0, false));
}

TEST_F(DispatcherTests, CheckWrongResponder)
{
  Payload   response("hello");
  PacketPtr packet = CreatePacket(1, 2, 3, response);

  // generate dummy address
  Packet::Address address;

  // register the exchange
  Promise prom = dispatcher_->RegisterExchange(1, 2, 3, address);

  dispatcher_->Dispatch(packet);

  EXPECT_TRUE(prom->IsWaiting());
  EXPECT_FALSE(prom->IsFailed());
  EXPECT_FALSE(prom->IsSuccessful());
}

TEST_F(DispatcherTests, CheckNeverResolved)
{
  // generate dummy address
  Packet::Address address;

  // register the exchange
  Promise prom = dispatcher_->RegisterExchange(1, 2, 3, address);

  // emulate the clearnup happening in the future
  auto now = Dispatcher::Clock::now() + std::chrono::hours{2};
  dispatcher_->Cleanup(now);

  EXPECT_FALSE(prom->IsWaiting());
  EXPECT_TRUE(prom->IsFailed());
  EXPECT_FALSE(prom->IsSuccessful());
  EXPECT_FALSE(prom->Wait(0, false));
}

TEST_F(DispatcherTests, CheckConnectionFailure)
{
  // generate dummy address
  Packet::Address address;

  // register the exchange
  Promise prom = dispatcher_->RegisterExchange(1, 2, 3, address);

  // informathe dispatch about the connection information
  dispatcher_->NotifyMessage(4, 1, 2, 3);

  // tell the dispatcher that the connection has died
  dispatcher_->NotifyConnectionFailure(4);

  EXPECT_FALSE(prom->IsWaiting());
  EXPECT_TRUE(prom->IsFailed());
  EXPECT_FALSE(prom->IsSuccessful());
  EXPECT_FALSE(prom->Wait(0, false));
}
