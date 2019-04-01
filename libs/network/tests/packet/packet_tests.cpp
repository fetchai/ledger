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
#include "network/muddle/packet.hpp"

#include <gmock/gmock.h>
#include <memory>

class PacketTests : public ::testing::Test
{
protected:
  using Packet    = fetch::muddle::Packet;
  using PacketPtr = std::shared_ptr<Packet>;
  using Payload   = Packet::Payload;
  using Prover    = fetch::crypto::ECDSASigner;

  void SetUp() override
  {
    prover_ = Prover();
    prover_.GenerateKeys();
    response_ = "hello";
    packet_   = CreatePacket(prover_.identity().identifier(), 1, 2, 3, response_);
  }

  PacketPtr CreatePacket(fetch::byte_array::ConstByteArray const &address, uint16_t service,
                         uint16_t channel, uint16_t counter, Payload const &payload)
  {
    PacketPtr packet_ = std::make_shared<Packet>(address, 0);
    packet_->SetService(service);
    packet_->SetProtocol(channel);
    packet_->SetMessageNum(counter);
    packet_->SetPayload(payload);

    return packet_;
  }

  Prover    prover_;
  Payload   response_;
  PacketPtr packet_;
};

TEST_F(PacketTests, CheckInvaldation)
{
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetDirect();
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetBroadcast();
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetExchange();
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetService(42);
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetProtocol(42);
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetMessageNum(42);
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetTarget(Packet::RawAddress{});
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetPayload(Payload{"Bye!"});
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  packet_->SetNetworkId(42);
  EXPECT_FALSE(packet_->IsStamped());
  EXPECT_FALSE(packet_->Verify());
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());
}

TEST_F(PacketTests, CheckIndifference)
{
  packet_->Sign(prover_);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());

  for (uint8_t ttl{255}; ttl > 0; --ttl)
  {
    packet_->SetTTL(ttl);
    EXPECT_TRUE(packet_->IsStamped());
    EXPECT_TRUE(packet_->Verify());
  }
  packet_->SetTTL(0);
  EXPECT_TRUE(packet_->IsStamped());
  EXPECT_TRUE(packet_->Verify());
}
