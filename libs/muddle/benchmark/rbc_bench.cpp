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

#include "muddle/punishment_broadcast_channel.hpp"
#include "muddle/rbc.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "core/reactor.hpp"
#include "crypto/ecdsa.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

#include "benchmark/benchmark.h"

#include "muddle/create_muddle_fake.hpp"

#include <type_traits>
#include <vector>

using namespace fetch;
using namespace fetch::muddle;
using namespace fetch::core;
using namespace fetch::crypto;

// Whether to use fake muddles for the benchmark
constexpr bool USING_FAKE_MUDDLES{true};

using fetch::crypto::Prover;

ProverPtr CreateNewCertificate()
{
  using Signer    = fetch::crypto::ECDSASigner;
  using SignerPtr = std::shared_ptr<Signer>;

  SignerPtr certificate = std::make_shared<Signer>();

  certificate->GenerateKeys();

  return certificate;
}

struct AbstractRBCNode
{
  using CertificatePtr = std::shared_ptr<Prover>;
  using Muddle         = fetch::muddle::MuddlePtr;
  using ConstByteArray = byte_array::ConstByteArray;
  using MuddleAddress  = byte_array::ConstByteArray;
  using MessageType    = byte_array::ConstByteArray;

  AbstractRBCNode(uint16_t port_number, uint16_t index)
    : muddle_port{port_number}
    , network_manager{"NetworkManager" + std::to_string(index), 2}
    , reactor{"ReactorName" + std::to_string(index)}
    , muddle_certificate{CreateNewCertificate()}
  {
    if (USING_FAKE_MUDDLES)
    {
      muddle = muddle::CreateMuddleFake("Test", muddle_certificate, network_manager, "127.0.0.1");
    }
    else
    {
      muddle = muddle::CreateMuddle("Test", muddle_certificate, network_manager, "127.0.0.1");
    }
  }

  void Start()
  {
    if (!USING_FAKE_MUDDLES)
    {
      network_manager.Start();
    }
    muddle->Start({muddle_port});
  }

  virtual ~AbstractRBCNode()
  {
    reactor.Stop();
    muddle->Stop();
    network_manager.Stop();
  }

  muddle::Address GetMuddleAddress() const
  {
    return muddle->GetAddress();
  }

  network::Uri GetHint() const
  {
    return fetch::network::Uri{"tcp://127.0.0.1:" + std::to_string(muddle_port)};
  }

  uint64_t MessagesReceived()
  {
    FETCH_LOCK(mutex);
    return answers.size();
  }

  void Clear()
  {
    FETCH_LOCK(mutex);
    answers.clear();
  }

  virtual void ResetCabinet(RBC::CabinetMembers const &members) = 0;
  virtual void SendMessage()                                    = 0;
  virtual void Enable(bool enable)                              = 0;
  virtual void PrepareForTest(uint16_t test)                    = 0;

  uint16_t                             muddle_port;
  network::NetworkManager              network_manager;
  core::Reactor                        reactor;
  ProverPtr                            muddle_certificate;
  Muddle                               muddle;
  std::mutex                           mutex;
  std::map<MuddleAddress, MessageType> answers;
  bool                                 muddle_is_fake{true};
};

struct RBCNode : public AbstractRBCNode
{
  using RBC                                 = fetch::muddle::RBC;
  static constexpr const char *LOGGING_NAME = "RBCNode";

  RBCNode(uint16_t port_number, uint16_t index)
    : AbstractRBCNode(port_number, index)
    , rbc{muddle->GetEndpoint(), muddle_certificate->identity().identifier(),
          [this](MuddleAddress const &from, ConstByteArray const &payload) -> void {
            FETCH_LOCK(mutex);
            answers[from] = payload;
          },
          nullptr}
  {}

  ~RBCNode() override
  {
    reactor.Stop();
  }

  void ResetCabinet(RBC::CabinetMembers const &members) override
  {
    rbc.ResetCabinet(members);
  }

  void SendMessage() override
  {
    rbc.Broadcast(MessageType(std::to_string(muddle_port)));
  }

  void Enable(bool enable) override
  {
    rbc.Enable(enable);
  }

  void PrepareForTest(uint16_t /*test*/) override
  {}

  RBC rbc;
};

struct PBCNode : public AbstractRBCNode
{
  using PBC                                 = fetch::muddle::PunishmentBroadcastChannel;
  static constexpr const char *LOGGING_NAME = "PBCNode";

  PBCNode(uint16_t port_number, uint16_t index)
    : AbstractRBCNode(port_number, index)
    , punishment_broadcast_channel{
          muddle->GetEndpoint(), muddle_certificate->identity().identifier(),
          [this](MuddleAddress const &from, ConstByteArray const &payload) -> void {
            FETCH_LOCK(mutex);
            answers[from] = payload;
          },
          muddle_certificate, 0}
  {
    reactor.Attach(punishment_broadcast_channel.GetRunnable());
    reactor.Start();
  }

  ~PBCNode() override
  {
    reactor.Stop();
  }

  void ResetCabinet(RBC::CabinetMembers const &members) override
  {
    punishment_broadcast_channel.ResetCabinet(members);
  }

  void SendMessage() override
  {
    std::string question = "What is your answer to: " + std::to_string(iteration);
    std::string answer =
        "Answer: " + std::to_string(muddle_port) + " rnd: " + std::to_string(iteration);
    punishment_broadcast_channel.SetQuestion(question, answer);
  }

  void Enable(bool enable) override
  {
    punishment_broadcast_channel.Enable(enable);
  }

  void PrepareForTest(uint16_t test_attempt) override
  {
    iteration = test_attempt;
  }

  uint16_t iteration = 0;
  PBC      punishment_broadcast_channel;
};

// Test either the PBCNode or the RBCNode
template <class RBC_TYPE>
void DKGWithEcho(benchmark::State &state)
{
  char const *LOGGING_NAME    = RBC_TYPE::LOGGING_NAME;
  bool        REUSING_MUDDLES = true;

  // The reliable broadcast channel needs the network to be torn down since it's hard
  // to guarantee messages aren't in flight between test iterations
  if (std::is_same<RBC_TYPE, RBCNode>::value || USING_FAKE_MUDDLES)
  {
    REUSING_MUDDLES = false;
  }

  // Suppress logging
  SetGlobalLogLevel(LogLevel::ERROR);

  uint16_t unique_port  = 8000;
  uint16_t test_attempt = 0;

  std::vector<std::unique_ptr<AbstractRBCNode>> nodes;

  for (auto _ : state)
  {
    RBC::CabinetMembers cabinet;

    auto nodes_in_test = uint64_t(state.range(0));

    FETCH_LOG_INFO(LOGGING_NAME, "===============================");
    FETCH_LOG_INFO(LOGGING_NAME, "Starting test: ", nodes_in_test);

    // Setup here - not included in test timing
    {
      state.PauseTiming();

      if (!REUSING_MUDDLES)
      {
        nodes.clear();
      }

      nodes.resize(nodes_in_test);
      test_attempt++;

      for (uint16_t i = 0; i < nodes_in_test; ++i)
      {
        auto &node = nodes[i];
        if (!node)
        {
          node = std::make_unique<RBC_TYPE>(unique_port + i, i);
          node->Start();
        }
        node->Clear();
        node->PrepareForTest(test_attempt);
        cabinet.insert(node->muddle_certificate->identity().identifier());

        for (uint16_t j = 0; j < i; j++)
        {
          node->muddle->ConnectTo(nodes[j]->GetMuddleAddress(), nodes[j]->GetHint());
        }
      }

      for (auto const &member : cabinet)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Cabinet member: ", member.ToBase64());
      }

      for (auto const &node : nodes)
      {
        node->ResetCabinet(cabinet);
      }

      // Wait until fully connected
      while (true)
      {
        bool connected = true;

        for (auto const &node : nodes)
        {
          if (node->muddle->GetNumDirectlyConnectedPeers() != nodes_in_test - 1)
          {
            connected = false;
          }
        }

        if (connected)
        {
          break;
        }
      }

      state.ResumeTiming();
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Sending messages");

    // Send all messages
    for (auto const &node : nodes)
    {
      node->SendMessage();
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Sent messages");

    // Wait until all have seen all messages
    while (true)
    {
      bool all_seen = true;

      for (auto const &node : nodes)
      {
        if (node->MessagesReceived() != nodes_in_test - 1)
        {
          all_seen = false;
        }
      }

      if (all_seen)
      {
        break;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Cleanup
    if (REUSING_MUDDLES)
    {
      state.PauseTiming();

      for (auto const &node : nodes)
      {
        node->Enable(false);
      }
      FETCH_LOG_INFO(LOGGING_NAME, "Disabled comms.");

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      for (auto const &node : nodes)
      {
        node->Enable(true);
      }
      FETCH_LOG_INFO(LOGGING_NAME, "Enabled comms");

      state.ResumeTiming();
    }
    else
    {
      state.PauseTiming();
      nodes.clear();
      if (!USING_FAKE_MUDDLES)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
      state.ResumeTiming();
    }

    // SetGlobalLogLevel(LogLevel::INFO);
    FETCH_LOG_INFO(LOGGING_NAME, "Finished test: ", nodes_in_test);
    FETCH_LOG_INFO(LOGGING_NAME, "");
    // SetGlobalLogLevel(LogLevel::ERROR);
  }
}

BENCHMARK_TEMPLATE(DKGWithEcho, PBCNode)->Range(4, 100)->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(DKGWithEcho, RBCNode)->Range(4, 64)->Unit(benchmark::kMillisecond);
