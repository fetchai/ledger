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

#include "core/serializers/counter.hpp"
#include "core/serializers/main_serializer.hpp"
#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "dkg/rbc.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace fetch;
using namespace fetch::network;
using namespace fetch::crypto;
using namespace fetch::muddle;
using namespace fetch::dkg;

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;
using Address        = fetch::muddle::Packet::Address;
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
            const std::vector<Failures> &failures = {})
    : RBC{endpoint, std::move(address), std::move(broadcast_callback)}
  {
    for (auto f : failures)
    {
      failures_flags_.set(static_cast<uint32_t>(f));
    }
  }

  void SendRBroadcast(SerialisedMessage const &msg, uint8_t num_messages)
  {
    uint32_t sender_index = id_;
    uint8_t  channel      = CHANNEL_BROADCAST;
    auto     counter      = static_cast<uint8_t>(msg_counter_ + 1);
    if (Failure(Failures::WRONG_CHANNEL))
    {
      ++channel;
    }
    else if (Failure(Failures::OUT_OF_SEQUENCE_MSGS))
    {
      assert(num_messages >= msg_counter_);
      counter = static_cast<uint8_t>(num_messages - msg_counter_);
    }
    RBroadcast  broadcast_msg{channel, sender_index, counter, msg};
    RBCEnvelope env{broadcast_msg};
    Broadcast(env);
    ++msg_counter_;
    OnRBroadcast(broadcast_msg, id_);  // Self sending
  }

private:
  std::bitset<static_cast<uint8_t>(Failures::WRONG_RANK) + 1> failures_flags_;

  bool Failure(Failures f) const
  {
    return failures_flags_[static_cast<uint8_t>(f)];
  }

  void SendBadAnswer(RBCEnvelope const &env)
  {
    auto        answer_ptr = std::dynamic_pointer_cast<RAnswer>(env.Message());
    std::string new_msg    = "Goodbye";
    fetch::serializers::MsgPackSerializer serialiser;
    serialiser << new_msg;
    RAnswer     new_answer{CHANNEL_BROADCAST, answer_ptr->id(), answer_ptr->counter(),
                       serialiser.data()};
    RBCEnvelope new_env{new_answer};

    RBCSerializerCounter new_env_counter;
    new_env_counter << new_env;

    RBCSerializer new_env_serializer;
    new_env_serializer.Reserve(new_env_counter.size());
    new_env_serializer << new_env;

    endpoint_.Broadcast(SERVICE_DKG, CHANNEL_BROADCAST, new_env_serializer.data());
  }

  void SendUnrequestedAnswer(RBCEnvelope const &env)
  {
    assert(env.Message()->type() == RBCMessage::MessageType::RECHO);
    auto        echo_ptr = std::dynamic_pointer_cast<REcho>(env.Message());
    std::string new_msg  = "Hello";
    fetch::serializers::MsgPackSerializer serialiser;
    serialiser << new_msg;
    RAnswer new_answer{CHANNEL_BROADCAST, echo_ptr->id(), echo_ptr->counter(), serialiser.data()};
    RBCEnvelope new_env{new_answer};

    RBCSerializerCounter new_env_counter;
    new_env_counter << new_env;

    RBCSerializer new_env_serializer;
    new_env_serializer.Reserve(new_env_counter.size());
    new_env_serializer << new_env;

    endpoint_.Broadcast(SERVICE_DKG, CHANNEL_BROADCAST, new_env_serializer.data());
  }

  void Broadcast(RBCEnvelope const &env) override
  {
    // Serialise the RBCEnvelope
    RBCSerializerCounter env_counter;
    env_counter << env;

    RBCSerializer env_serializer;
    env_serializer.Reserve(env_counter.size());
    env_serializer << env;

    if ((Failure(Failures::NO_ECHO) && env.Message()->type() == RBCMessage::MessageType::RECHO) or
        (Failure(Failures::NO_READY) && env.Message()->type() == RBCMessage::MessageType::RREADY) or
        (Failure(Failures::NO_ANSWER) && env.Message()->type() == RBCMessage::MessageType::RANSWER))
    {
      return;
    }
    else if (Failure(Failures::DOUBLE_SEND))
    {
      endpoint_.Broadcast(SERVICE_DKG, CHANNEL_BROADCAST, env_serializer.data());
    }
    else if (Failure(Failures::BAD_ANSWER) &&
             env.Message()->type() == RBCMessage::MessageType::RANSWER)
    {
      SendBadAnswer(env);
      return;
    }
    else if (Failure(Failures::UNREQUESTED_ANSWER) &&
             env.Message()->type() == RBCMessage::MessageType::RECHO)
    {
      SendUnrequestedAnswer(env);
    }
    endpoint_.Broadcast(SERVICE_DKG, CHANNEL_BROADCAST, env_serializer.data());
  }

  void OnRBC(MuddleAddress const &from, RBCEnvelope const &envelope) override
  {
    auto msg_ptr = envelope.Message();
    if (!BasicMsgCheck(from, msg_ptr))
    {
      return;
    }
    uint32_t sender_index{CabinetIndex(from)};
    switch (msg_ptr->type())
    {
    case RBCMessage::MessageType::RBROADCAST:
    {
      auto broadcast_ptr = std::dynamic_pointer_cast<RBroadcast>(msg_ptr);
      if (broadcast_ptr != nullptr)
      {
        auto payload = broadcast_ptr->message();
        auto index   = broadcast_ptr->id();
        if (Failure(Failures::BAD_MESSAGE))
        {
          std::string                           new_msg = "Goodbye";
          fetch::serializers::MsgPackSerializer serialiser;
          serialiser << new_msg;
          payload = serialiser.data();
        }
        else if (Failure(Failures::WRONG_RANK))
        {
          index = static_cast<uint32_t>((broadcast_ptr->id() + 1) % current_cabinet_.size());
        }
        RBroadcast new_broadcast{CHANNEL_BROADCAST, index, broadcast_ptr->counter(), payload};
        OnRBroadcast(new_broadcast, sender_index);
      }
      break;
    }
    case RBCMessage::MessageType::RECHO:
    {
      auto echo_ptr = std::dynamic_pointer_cast<REcho>(msg_ptr);
      if (echo_ptr != nullptr)
      {
        OnREcho(*echo_ptr, sender_index);
      }
      break;
    }
    case RBCMessage::MessageType::RREADY:
    {
      auto ready_ptr = std::dynamic_pointer_cast<RReady>(msg_ptr);
      if (ready_ptr != nullptr)
      {
        OnRReady(*ready_ptr, sender_index);
      }
      break;
    }
    case RBCMessage::MessageType::RREQUEST:
    {
      auto request_ptr = std::dynamic_pointer_cast<RRequest>(msg_ptr);
      if (request_ptr != nullptr)
      {
        OnRRequest(*request_ptr, sender_index);
      }
      break;
    }
    case RBCMessage::MessageType::RANSWER:
    {
      auto answer_ptr = std::dynamic_pointer_cast<RAnswer>(msg_ptr);
      if (answer_ptr != nullptr)
      {
        OnRAnswer(*answer_ptr, sender_index);
      }
      break;
    }
    default:
      std::cerr << "Invalid payload. Can not process" << std::endl;
    }
  }
};

struct RbcMember
{
  uint16_t              muddle_port;
  NetworkManager        network_manager;
  ProverPtr             muddle_certificate;
  Muddle                muddle;
  RBC::CabinetMembers   cabinet;
  FaultyRbc             rbc;
  std::atomic<uint16_t> delivered_msgs{0};

  RbcMember(uint16_t port_number, uint16_t index,
            const std::vector<FaultyRbc::Failures> &failures = {})
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 1}
    , muddle_certificate{CreateCertificate()}
    , muddle{fetch::muddle::NetworkId{"TestNetwork"}, muddle_certificate, network_manager}
    , rbc{muddle.AsEndpoint(), muddle_certificate->identity().identifier(),
          [this](ConstByteArray const &, ConstByteArray const &payload) -> void {
            OnRbcMessage(payload);
          },
          failures}
  {
    network_manager.Start();
    muddle.Start({muddle_port});
  }

  ~RbcMember()
  {
    muddle.Stop();
    muddle.Shutdown();
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

  void ResetCabinet(RBC::CabinetMembers const &new_cabinet)
  {
    cabinet = new_cabinet;
    rbc.ResetCabinet(cabinet);
  }
};

void GenerateRbcTest(uint32_t cabinet_size, uint32_t expected_completion_size,
                     const std::vector<std::vector<FaultyRbc::Failures>> &failures     = {},
                     uint8_t                                              num_messages = 1)
{

  std::vector<std::unique_ptr<RbcMember>> committee;
  for (uint16_t ii = 0; ii < cabinet_size; ++ii)
  {
    auto port_number = static_cast<uint16_t>(9000 + ii);
    if (!failures.empty() && ii < failures.size())
    {
      committee.emplace_back(new RbcMember{port_number, ii, failures[ii]});
    }
    else
    {
      committee.emplace_back(new RbcMember{port_number, ii});
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Connect muddles together (localhost for this example)
  for (uint32_t ii = 0; ii < cabinet_size; ii++)
  {
    for (uint32_t jj = ii + 1; jj < cabinet_size; jj++)
    {
      committee[ii]->muddle.AddPeer(
          fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(committee[jj]->muddle_port)});
    }
  }

  // Make sure everyone is connected to everyone else
  uint32_t kk = 0;
  while (kk != cabinet_size)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (uint32_t mm = kk; mm < cabinet_size; ++mm)
    {
      if (committee[mm]->muddle.AsEndpoint().GetDirectlyConnectedPeers().size() !=
          (cabinet_size - 1))
      {
        break;
      }
      else
      {
        ++kk;
      }
    }
  }

  RBC::CabinetMembers cabinet;
  for (auto &member : committee)
  {
    cabinet.insert(member->muddle_certificate->identity().identifier());
  }
  assert(cabinet.size() == cabinet_size);

  // Reset cabinet
  for (auto &member : committee)
  {
    member->ResetCabinet(cabinet);
  }

  // First node sends a broadcast
  {
    std::string                           msg = "Hello";
    fetch::serializers::MsgPackSerializer serialiser;
    serialiser << msg;
    uint32_t sender_index = cabinet_size - 1;
    for (auto ii = 0; ii < num_messages; ++ii)
    {
      committee[sender_index]->rbc.SendRBroadcast(serialiser.data(), num_messages);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1 * num_messages));
    uint32_t pp = 0;
    for (uint32_t qq = 0; qq < cabinet_size; ++qq)
    {
      if (qq != sender_index && committee[qq]->delivered_msgs == num_messages)
      {
        ++pp;
      }
    }
    EXPECT_EQ(pp, expected_completion_size);
  }
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
  GenerateRbcTest(4, 3,
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
