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

#include "core/byte_array/decoders.hpp"
#include "crypto/ecdsa.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "network/management/network_manager.hpp"
#include "network/peer.hpp"
#include "network/service/protocol.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>

using std::this_thread::sleep_for;
using std::chrono::seconds;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;
using fetch::muddle::NetworkId;

class TestProtocol : public fetch::service::Protocol
{
public:
  enum
  {
    EXCHANGE = 0xEF
  };

  TestProtocol()
  {
    Expose(EXCHANGE, this, &TestProtocol::Exchange);
  }

private:
  ConstByteArray Exchange(ConstByteArray const &value)
  {
    return value;
  }
};

static constexpr uint16_t SERVICE = 10;
static constexpr uint16_t CHANNEL = 12;

class MuddleRpcStressTests : public ::testing::Test
{
protected:
  static constexpr char const *NETWORK_A_PUBLIC_KEY =
      "rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==";
  static constexpr char const *NETWORK_A_PRIVATE_KEY =
      "BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=";
  static constexpr char const *NETWORK_B_PUBLIC_KEY =
      "646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==";
  static constexpr char const *NETWORK_B_PRIVATE_KEY =
      "4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=";

  using NetworkManager    = fetch::network::NetworkManager;
  using NetworkManagerPtr = std::unique_ptr<NetworkManager>;
  using Muddle            = fetch::muddle::Muddle;
  using MuddlePtr         = std::shared_ptr<Muddle>;
  using MuddleEndpoint    = fetch::muddle::MuddleEndpoint;
  using CertificatePtr    = Muddle::CertificatePtr;
  using Peer              = fetch::network::Peer;
  using Uri               = Muddle::Uri;
  using UriList           = Muddle::UriList;
  using RpcServer         = fetch::muddle::rpc::Server;
  using RpcClient         = fetch::muddle::rpc::Client;
  using Flag              = std::atomic<bool>;
  using Promise           = fetch::service::Promise;

  static CertificatePtr LoadIdentity(char const *private_key)
  {
    using Signer = fetch::crypto::ECDSASigner;

    // load the key
    auto signer = std::make_unique<Signer>();
    signer->Load(FromBase64(private_key));

    return signer;
  }

  void SetUp() override
  {
    managerA_ = std::make_unique<NetworkManager>("NetMgrA", 1);
    networkA_ = std::make_shared<Muddle>(NetworkId{"Test"}, LoadIdentity(NETWORK_A_PRIVATE_KEY),
                                         *managerA_);

    managerB_ = std::make_unique<NetworkManager>("NetMgrB", 1);
    networkB_ = std::make_shared<Muddle>(NetworkId{"Test"}, LoadIdentity(NETWORK_B_PRIVATE_KEY),
                                         *managerB_);

    managerA_->Start();
    managerB_->Start();

    networkA_->Start({8000});
    networkB_->Start({"tcp://127.0.0.1:8000"}, {9000});

    sleep_for(seconds{1});
  }

  void TearDown() override
  {
    networkB_->Stop();
    networkA_->Stop();
    managerB_->Stop();
    managerA_->Stop();

    networkB_.reset();
    managerB_.reset();

    networkA_.reset();
    managerA_.reset();
  }

  static ConstByteArray GenerateData(std::size_t length, uint8_t fill)
  {
    ByteArray buffer;
    buffer.Resize(length);
    for (std::size_t i = 0; i < length; ++i)
    {
      buffer[i] = fill;
    }
    return {std::move(buffer)};
  }

  static void ClientServer(MuddleEndpoint &endpoint, char const *target)
  {
    static constexpr std::size_t NUM_MESSAGES   = 200;
    static constexpr std::size_t PAYLOAD_LENGTH = 5;
    static constexpr uint64_t    PROTOCOL       = 0xEF;

    // create the server
    TestProtocol protocol;
    auto         server = std::make_shared<RpcServer>(endpoint, SERVICE, CHANNEL);
    server->Add(PROTOCOL, &protocol);

    // create the client
    auto client = std::make_shared<RpcClient>("Client", endpoint, SERVICE, CHANNEL);

    if (NETWORK_A_PUBLIC_KEY == target)
    {
      sleep_for(seconds{2});
    }

    std::vector<Promise> pending;
    for (std::size_t loop = 0; loop < NUM_MESSAGES; ++loop)
    {
      // generate a big load of data
      auto const           fill = static_cast<uint8_t>(loop);
      ConstByteArray const data = GenerateData(PAYLOAD_LENGTH, fill);

      Promise promise =
          client->CallSpecificAddress(FromBase64(target), PROTOCOL, TestProtocol::EXCHANGE, data);
      promise->WithHandlers()
          .Then([promise, fill]() {
            ConstByteArray result{};
            ASSERT_TRUE(promise->GetResult(result));
            EXPECT_EQ(PAYLOAD_LENGTH, result.size());

            ConstByteArray const &result_ref{result};
            for (std::size_t i = 0; i < result.size(); ++i)
            {
              EXPECT_EQ(result_ref[i], fill);
            }
          })
          .Catch([]() { FAIL(); });

      pending.push_back(promise);
    }

    while (!pending.empty())
    {
      if (!pending.back()->IsWaiting())
      {
        pending.pop_back();
        continue;
      }

      sleep_for(seconds{1});
    }

    sleep_for(seconds{5});
  }

  NetworkManagerPtr managerA_;
  MuddlePtr         networkA_;

  NetworkManagerPtr managerB_;
  MuddlePtr         networkB_;
};

TEST_F(MuddleRpcStressTests, DISABLED_ContinuousBiDirectionalTraffic)
{
  std::thread nodeA([this]() { ClientServer(networkA_->GetEndpoint(), NETWORK_B_PUBLIC_KEY); });
  std::thread nodeB([this]() { ClientServer(networkB_->GetEndpoint(), NETWORK_A_PUBLIC_KEY); });

  nodeB.join();
  nodeA.join();
}
