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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "network/muddle/subscription.hpp"
#include "network/muddle/subscription_registrar.hpp"
#include "network/uri.hpp"

#include "gmock/gmock.h"

#include <cstdint>
#include <memory>
#include <utility>

static const auto SAMPLE_ADDRESS = fetch::byte_array::FromBase64(
    "wvV0DQgjcMNsmtkTTTZtX0JSAGA9+bHi7iRTczWDZsVJznK4c5enNJFSUyZScG40D3Dp2gdpT2WmnZO1lkUheQ==");

class SubscriptionManagerTests : public ::testing::Test
{
protected:
  using Packet                   = fetch::muddle::Packet;
  using Address                  = Packet::Address;
  using PacketPtr                = std::shared_ptr<Packet>;
  using SubscriptionRegistrar    = fetch::muddle::SubscriptionRegistrar;
  using SubscriptionRegistrarPtr = std::unique_ptr<SubscriptionRegistrar>;

  void SetUp() override
  {
    registrar_ = std::make_unique<SubscriptionRegistrar>();
  }

  PacketPtr CreatePacket(uint16_t service, uint16_t channel,
                         Packet::Address const &address = Packet::Address{})
  {
    auto packet = std::make_shared<Packet>();

    packet->SetService(service);
    packet->SetProtocol(channel);

    if (address.size() > 0)
    {
      packet->SetTarget(address);
    }

    return packet;
  }

  SubscriptionRegistrarPtr registrar_;
};

TEST_F(SubscriptionManagerTests, SingleHandler)
{
  auto subscription = registrar_->Register(1, 2);

  // register the message handler for the subscription
  uint32_t dispatches = 0;
  subscription->SetMessageHandler([&dispatches](Address const &, uint16_t, uint16_t, uint16_t,
                                                Packet::Payload const &,
                                                Address const &) { ++dispatches; });

  // create the packet
  auto packet = CreatePacket(1, 2);

  EXPECT_EQ(dispatches, 0);

  // dispatch the packet to the registrar
  registrar_->Dispatch(packet, Address());

  EXPECT_EQ(dispatches, 1);

  // dispatch the packet to the registrar
  registrar_->Dispatch(packet, Address());

  EXPECT_EQ(dispatches, 2);
}

TEST_F(SubscriptionManagerTests, MultipleHandlers)
{
  auto subscription1 = registrar_->Register(1, 2);
  auto subscription2 = registrar_->Register(1, 2);

  // register the message handler for the subscription
  uint32_t dispatches = 0;
  subscription1->SetMessageHandler([&dispatches](Address const &, uint16_t, uint16_t, uint16_t,
                                                 Packet::Payload const &,
                                                 Address const &) { ++dispatches; });
  subscription2->SetMessageHandler([&dispatches](Address const &, uint16_t, uint16_t, uint16_t,
                                                 Packet::Payload const &,
                                                 Address const &) { ++dispatches; });

  // create the packet
  auto packet = CreatePacket(1, 2);

  EXPECT_EQ(dispatches, 0);

  // dispatch the packet to the registrar
  registrar_->Dispatch(packet, Address());

  EXPECT_EQ(dispatches, 2);

  // dispatch the packet to the registrar
  registrar_->Dispatch(packet, Address());

  EXPECT_EQ(dispatches, 4);

  // cancel the subscription
  subscription2.reset();

  // dispatch the packet to the registrar
  registrar_->Dispatch(packet, Address());

  EXPECT_EQ(dispatches, 5);
}

TEST_F(SubscriptionManagerTests, MultipleDifferentHandlers)
{
  auto subscription1 = registrar_->Register(1, 2);
  auto subscription2 = registrar_->Register(SAMPLE_ADDRESS, 1, 2);

  // register the message handler for the subscription
  uint32_t dispatches = 0;
  subscription1->SetMessageHandler([&dispatches](Address const &, uint16_t, uint16_t, uint16_t,
                                                 Packet::Payload const &,
                                                 Address const &) { ++dispatches; });
  subscription2->SetMessageHandler([&dispatches](Address const &, uint16_t, uint16_t, uint16_t,
                                                 Packet::Payload const &,
                                                 Address const &) { ++dispatches; });

  // create the packet
  auto packet = CreatePacket(1, 2, SAMPLE_ADDRESS);

  EXPECT_EQ(dispatches, 0);

  // dispatch the packet to the registrar
  registrar_->Dispatch(packet, Address());

  EXPECT_EQ(dispatches, 2);

  // dispatch the packet to the registrar
  registrar_->Dispatch(packet, Address());

  EXPECT_EQ(dispatches, 4);

  // cancel the subscription
  subscription2.reset();

  // dispatch the packet to the registrar
  registrar_->Dispatch(packet, Address());

  EXPECT_EQ(dispatches, 5);
}
