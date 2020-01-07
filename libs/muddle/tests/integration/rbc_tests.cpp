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

#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rbc.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <memory>

using namespace fetch;
using namespace fetch::network;
using namespace fetch::crypto;
using namespace fetch::muddle;

using Prover         = fetch::crypto::Prover;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
// using Address        = fetch::muddle::Packet::Address;
using ConstByteArray = fetch::byte_array::ConstByteArray;

ProverPtr CreateCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

class FaultyRbc : public RBC
{
public:
  enum class Failures : uint8_t
  {
    BAD_MESSAGE,
    NO_ECHO,
    NO_READY,
    NO_ANSWER,
    BAD_ANSWER,
    DOUBLE_SEND,
    UNREQUESTED_ANSWER,
    WRONG_CHANNEL,
    OUT_OF_SEQUENCE_MSGS,
    WRONG_RANK
  };
  FaultyRbc(Endpoint &endpoint, MuddleAddress address,
            std::function<void(ConstByteArray const &address, ConstByteArray const &payload)>
                                         broadcast_callback,
            const std::vector<Failures> &failure)
    : RBC{endpoint, std::move(address), std::move(broadcast_callback)}
  {
    for (auto f : failure)
    {
      failures_flags_.set(static_cast<uint32_t>(f));
    }
  }

  void Broadcast(SerialisedMessage const &msg, uint8_t num_messages)
  {
    FETCH_LOCK(lock_);

    uint32_t sender_index = this->id();
    uint16_t channel      = CHANNEL_RBC_BROADCAST;
    auto     counter      = static_cast<uint8_t>(this->message_counter() + 1);
    if (Failure(Failures::WRONG_CHANNEL))
    {
      ++channel;
    }
    else if (Failure(Failures::OUT_OF_SEQUENCE_MSGS))
    {
      assert(num_messages >= this->message_counter());
      counter = static_cast<uint8_t>(num_messages - this->message_counter());
    }
    MessageBroadcast broadcast_msg =
        RBCMessage::New<RBroadcast>(channel, sender_index, counter, msg);
    InternalBroadcast(*broadcast_msg);

    increase_message_counter();
    OnRBroadcast(broadcast_msg, this->id());  // Self sending
  }

private:
  std::bitset<static_cast<uint8_t>(Failures::WRONG_RANK) + 1> failures_flags_;

  bool Failure(Failures f) const
  {
    return failures_flags_[static_cast<uint8_t>(f)];
  }

  void SendBadAnswer(RBCMessage const &msg, MuddleAddress const &address)
  {
    std::string                           new_msg = "Goodbye";
    fetch::serializers::MsgPackSerializer serialiser;
    serialiser << new_msg;
    RAnswer new_rmsg{CHANNEL_RBC_BROADCAST, msg.id(), msg.counter(), serialiser.data()};

    RBCSerializerCounter new_rmsg_counter;
    new_rmsg_counter << static_cast<RBCMessage>(new_rmsg);

    RBCSerializer new_rmsg_serializer;
    new_rmsg_serializer.Reserve(new_rmsg_counter.size());
    new_rmsg_serializer << static_cast<RBCMessage>(new_rmsg);

    endpoint().Send(address, SERVICE_RBC, CHANNEL_RBC_BROADCAST, new_rmsg_serializer.data());
  }

  void SendUnrequestedAnswer(RBCMessage const &msg)
  {
    assert(msg.type() == RBCMessageType::R_ECHO);
    std::string                           new_msg = "Hello";
    fetch::serializers::MsgPackSerializer serialiser;
    serialiser << new_msg;
    RAnswer new_rmsg{CHANNEL_RBC_BROADCAST, msg.id(), msg.counter(), serialiser.data()};

    RBCSerializerCounter new_rmsg_counter;
    new_rmsg_counter << static_cast<RBCMessage>(new_rmsg);

    RBCSerializer new_rmsg_serializer;
    new_rmsg_serializer.Reserve(new_rmsg_counter.size());
    new_rmsg_serializer << static_cast<RBCMessage>(new_rmsg);

    endpoint().Broadcast(SERVICE_RBC, CHANNEL_RBC_BROADCAST, new_rmsg_serializer.data());
  }

  void Send(RBCMessage const &msg, MuddleAddress const &address) override
  {
    // Serialise the RBCEnvelope
    RBCSerializerCounter msg_counter;
    msg_counter << msg;

    RBCSerializer msg_serializer;
    msg_serializer.Reserve(msg_counter.size());
    msg_serializer << msg;

    if (Failure(Failures::BAD_ANSWER) && msg.type() == RBCMessageType::R_ANSWER)
    {
      SendBadAnswer(msg, address);
      return;
    }
    if (Failure(Failures::NO_ANSWER) && msg.type() == RBCMessageType::R_ANSWER)
    {
      return;
    }

    endpoint().Send(address, SERVICE_RBC, CHANNEL_RBC_BROADCAST, msg_serializer.data());
  }

  void InternalBroadcast(RBCMessage const &msg) override
  {
    // Serialise the RBCEnvelope
    RBCSerializerCounter msg_counter;
    msg_counter << msg;

    RBCSerializer msg_serializer;
    msg_serializer.Reserve(msg_counter.size());
    msg_serializer << msg;

    if ((Failure(Failures::NO_ECHO) && msg.type() == RBCMessageType::R_ECHO) ||
        (Failure(Failures::NO_READY) && msg.type() == RBCMessageType::R_READY))
    {
      return;
    }
    if (Failure(Failures::DOUBLE_SEND))
    {
      endpoint().Broadcast(SERVICE_RBC, CHANNEL_RBC_BROADCAST, msg_serializer.data());
    }
    else if (Failure(Failures::UNREQUESTED_ANSWER) && msg.type() == RBCMessageType::R_ECHO)
    {
      SendUnrequestedAnswer(msg);
    }
    endpoint().Broadcast(SERVICE_RBC, CHANNEL_RBC_BROADCAST, msg_serializer.data());
  }

  void OnRBC(MuddleAddress const &from, RBCMessage const &msg) override
  {
    FETCH_LOCK(lock_);
    if (!BasicMessageCheck(from, msg))
    {
      return;
    }
    uint32_t sender_index = CabinetIndex(from);

    switch (msg.type())
    {
    case RBCMessageType::R_BROADCAST:
    {

      auto payload = msg.message();
      auto index   = msg.id();
      if (Failure(Failures::BAD_MESSAGE))
      {
        std::string                           new_msg = "Goodbye";
        fetch::serializers::MsgPackSerializer serialiser;
        serialiser << new_msg;
        payload = serialiser.data();
      }
      else if (Failure(Failures::WRONG_RANK))
      {
        index = static_cast<uint32_t>((msg.id() + 1) % current_cabinet().size());
      }

      MessageBroadcast new_broadcast =
          RBCMessage::New<RBroadcast>(CHANNEL_RBC_BROADCAST, index, msg.counter(), payload);

      OnRBroadcast(new_broadcast, sender_index);

      break;
    }
    case RBCMessageType::R_ECHO:
    {
      OnREcho(RBCMessage::New<REcho>(msg), sender_index);
      break;
    }
    case RBCMessageType::R_READY:
    {
      OnRReady(RBCMessage::New<RReady>(msg), sender_index);
      break;
    }
    case RBCMessageType::R_REQUEST:
    {
      OnRRequest(RBCMessage::New<RRequest>(msg), sender_index);
      break;
    }
    case RBCMessageType::R_ANSWER:
    {
      OnRAnswer(RBCMessage::New<RAnswer>(msg), sender_index);
      break;
    }
    default:
      std::cerr << "Invalid payload. Can not process" << std::endl;
    }
  }
};

class RbcMember
{

public:
  uint16_t              muddle_port;
  ProverPtr             muddle_certificate;
  NetworkManager        network_manager;
  MuddlePtr             muddle;
  std::atomic<uint16_t> delivered_msgs{0};

  virtual ~RbcMember()
  {
    muddle->Stop();
    network_manager.Stop();
  }

  void OnRbcMessage(ConstByteArray const &payload)
  {
    fetch::serializers::MsgPackSerializer serializer{payload};
    std::string                           msg;
    serializer >> msg;
    assert(msg == "Hello");
    ++delivered_msgs;
  }
  muddle::Address GetMuddleAddress() const
  {
    return muddle->GetAddress();
  }

  network::Uri GetHint() const
  {
    return fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(muddle_port)};
  }

  virtual void ResetCabinet(RBC::CabinetMembers const &new_cabinet)          = 0;
  virtual void Broadcast(SerialisedMessage const &msg, uint8_t num_messages) = 0;

protected:
  RbcMember(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , muddle_certificate{CreateCertificate()}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , muddle{CreateMuddle("Test", muddle_certificate, network_manager, "127.0.0.1")}
  {
    network_manager.Start();
    muddle->Start({muddle_port});
  }
};

class FaultyRbcMember : public RbcMember
{
public:
  FaultyRbcMember(uint16_t port_number, uint16_t index,
                  const std::vector<FaultyRbc::Failures> &failure)
    : RbcMember{port_number, index}
    , rbc_{muddle->GetEndpoint(), muddle_certificate->identity().identifier(),
           [this](ConstByteArray const &, ConstByteArray const &payload) -> void {
             OnRbcMessage(payload);
           },
           failure}
  {}

  void ResetCabinet(RBC::CabinetMembers const &new_cabinet) override
  {
    rbc_.ResetCabinet(new_cabinet);
  }
  void Broadcast(SerialisedMessage const &msg, uint8_t num_messages) override
  {
    rbc_.Broadcast(msg, num_messages);
  }

private:
  FaultyRbc rbc_;
};

class HonestRbcMember : public RbcMember
{

public:
  HonestRbcMember(uint16_t port_number, uint16_t index)
    : RbcMember{port_number, index}
    , rbc_{muddle->GetEndpoint(), muddle_certificate->identity().identifier(),
           [this](ConstByteArray const &, ConstByteArray const &payload) -> void {
             OnRbcMessage(payload);
           }}
  {}

  void ResetCabinet(RBC::CabinetMembers const &new_cabinet) override
  {
    rbc_.ResetCabinet(new_cabinet);
  }
  void Broadcast(SerialisedMessage const &msg, uint8_t /*num_messages*/) override
  {
    rbc_.Broadcast(msg);
  }

private:
  RBC rbc_;
};

void GenerateRbcTest(uint32_t cabinet_size, uint32_t expected_completion_size,
                     const std::vector<std::vector<FaultyRbc::Failures>> &failures     = {},
                     uint8_t                                              num_messages = 1)
{

  RBC::CabinetMembers                     cabinet_members;
  std::vector<std::unique_ptr<RbcMember>> cabinet;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    if (ii < failures.size() && !failures[ii].empty())
    {
      cabinet.emplace_back(new FaultyRbcMember{port_number, ii, failures[ii]});
    }
    else
    {
      cabinet.emplace_back(new HonestRbcMember{port_number, ii});
    }
    cabinet_members.insert(cabinet[ii]->muddle_certificate->identity().identifier());
  }

  // Reset cabinet
  for (auto &member : cabinet)
  {
    member->ResetCabinet(cabinet_members);
  }

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    for (uint32_t jj = ii + 1; jj < cabinet_size; jj++)
    {
      cabinet[ii]->muddle->ConnectTo(cabinet[jj]->GetMuddleAddress(), cabinet[jj]->GetHint());
    }
  }

  // Make sure everyone is connected to everyone else
  uint32_t kk = 0, tt = 0;
  while (kk != cabinet_size)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (uint32_t mm = kk; mm < cabinet_size; ++mm)
    {
      if (cabinet[mm]->muddle->GetEndpoint().GetDirectlyConnectedPeers().size() !=
          (cabinet_size - 1))
      {
        break;
      }

      ++kk;
    }
    ++tt;
    if (tt > 200)
    {
      throw std::runtime_error("Time to setup exceeded.");
    }
  }

  // First node sends a broadcast
  {
    std::string                           msg = "Hello";
    fetch::serializers::MsgPackSerializer serialiser;
    serialiser << msg;
    uint32_t sender_index = cabinet_size - 1;
    for (auto ii = 0; ii < num_messages; ++ii)
    {
      cabinet[sender_index]->Broadcast(serialiser.data(), num_messages);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1 * num_messages));
    uint32_t pp = 0;
    for (uint32_t qq = 0; qq < cabinet_size; ++qq)
    {
      if (qq != sender_index && cabinet[qq]->delivered_msgs == num_messages)
      {
        ++pp;
      }
    }
    EXPECT_EQ(pp, expected_completion_size);
  }
}

TEST(rbc, all_honest)
{
  GenerateRbcTest(4, 3, {});
}

TEST(rbc, bad_message)
{
  // One node receives the wrong message and sends an echo with the wrong hash but
  // everyone should deliver the same message through requests
  GenerateRbcTest(4, 3, {{FaultyRbc::Failures::BAD_MESSAGE}});
}

TEST(rbc, no_echo)
{
  // One node withholds their echo message but there should be enough for everyone to proceed
  GenerateRbcTest(4, 3, {{FaultyRbc::Failures::NO_ECHO}});
}

TEST(rbc, no_ready)
{
  // One node withholds their ready message but there should be enough for everyone to proceed
  GenerateRbcTest(4, 3, {{FaultyRbc::Failures::NO_READY}});
}

TEST(rbc, no_answer)
{
  // One node withholds their answer message but there should be enough for everyone to proceed
  GenerateRbcTest(4, 3, {{FaultyRbc::Failures::BAD_MESSAGE}, {FaultyRbc::Failures::NO_ANSWER}});
}

TEST(rbc, too_many_no_answer)
{
  // Three nodes withhold their answer messages which excludes the node from delivering the message
  GenerateRbcTest(4, 2,
                  {{FaultyRbc::Failures::BAD_MESSAGE},
                   {FaultyRbc::Failures::NO_ANSWER},
                   {FaultyRbc::Failures::NO_ANSWER},
                   {FaultyRbc::Failures::NO_ANSWER}});
}

TEST(rbc, bad_answer)
{
  // One node receives the wrong message and sends request for the real message. Receives bad answer
  // from at least one peer but receives the correct message in the end. Everyone should deliver
  GenerateRbcTest(4, 3, {{FaultyRbc::Failures::BAD_MESSAGE}, {FaultyRbc::Failures::BAD_ANSWER}});
}

TEST(rbc, double_send)
{
  // One node sends all messages twice. Should raise warning.
  GenerateRbcTest(4, 3, {{FaultyRbc::Failures::DOUBLE_SEND}});
}

TEST(rbc, wrong_rank)
{
  // One node receives broadcast with incorrect rank
  GenerateRbcTest(4, 3, {{FaultyRbc::Failures::WRONG_RANK}});
}

TEST(rbc, unrequested_answer)
{
  // One node sends an unrequested answer
  GenerateRbcTest(4, 3, {{FaultyRbc::Failures::UNREQUESTED_ANSWER}});
}

TEST(rbc, wrong_channel)
{
  // One node sends message with the wrong channel
  GenerateRbcTest(4, 0, {{}, {}, {}, {FaultyRbc::Failures::WRONG_CHANNEL}});
}

TEST(rbc, out_of_order_messages)
{
  // Node 0 sends a sequence of messages but out of order
  GenerateRbcTest(4, 3, {{}, {}, {}, {FaultyRbc::Failures::OUT_OF_SEQUENCE_MSGS}}, 3);
}
