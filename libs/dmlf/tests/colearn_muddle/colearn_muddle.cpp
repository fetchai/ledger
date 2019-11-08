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

#include "gtest/gtest.h"

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "crypto/ecdsa.hpp"
#include "dmlf/colearn/muddle_learner_networker_impl.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "muddle/muddle_interface.hpp"
#include "network/management/network_manager.hpp"

#include <chrono>
#include <ostream>
#include <thread>
#include <unordered_set>

namespace {

using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;

using LNBase  = fetch::dmlf::AbstractLearnerNetworker;
using LN      = fetch::dmlf::colearn::MuddleLearnerNetworkerImpl;
using LNBaseP = std::shared_ptr<LNBase>;
using LNP     = std::shared_ptr<LN>;
using NetMan  = fetch::network::NetworkManager;
using NetManP = std::shared_ptr<NetMan>;

using MuddlePtr      = fetch::muddle::MuddlePtr;
using CertificatePtr = fetch::muddle::ProverPtr;

using UpdateTypeForTesting = fetch::dmlf::Update<TensorType>;
using UpdatePayload        = UpdateTypeForTesting::Payload;

char const *SERVER_PRIV = "BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=";
char const *SERVER_PUB =
    "rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==";
char const *CLIENT_PRIV = "4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=";
char const *CLIENT_PUB =
    "646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==";

class LearnerTypedUpdates
{
public:
  LNP         actual;
  LNBaseP     interface;
  NetManP     netm_;
  MuddlePtr   mud_;
  std::string pub_;

  LearnerTypedUpdates(const std::string &pub, const std::string &priv, unsigned short int port,
                      unsigned short int remote = 0)
  {
    pub_       = pub;
    auto ident = LoadIdentity(priv);
    netm_      = std::make_shared<NetMan>("LrnrNet", 4);
    netm_->Start();
    mud_ = fetch::muddle::CreateMuddle("Test", ident, *(this->netm_), "127.0.0.1");

    actual = std::make_shared<LN>(mud_);

    std::unordered_set<std::string> remotes;
    if (remote)
    {
      std::string server = "tcp://127.0.0.1:";
      server += std::to_string(remote);
      remotes.insert(server);
    }

    mud_->SetPeerSelectionMode(fetch::muddle::PeerSelectionMode::KADEMLIA);
    mud_->Start(remotes, {port});

    interface = actual;
    interface->RegisterUpdateType<UpdateTypeForTesting>("update");
    interface->RegisterUpdateType<fetch::dmlf::Update<std::string>>("vocab");
  }

  void PretendToLearn()
  {
    std::cout << "LEARN" << std::endl;
    static int sequence_number = 1;
    auto       t               = TensorType(TensorType::SizeType(2));
    t.Fill(DataType(sequence_number++));
    auto r = std::vector<TensorType>();
    r.push_back(t);
    interface->PushUpdateType("update", std::make_shared<UpdateTypeForTesting>(r));
    interface->PushUpdateType("vocab", std::make_shared<fetch::dmlf::Update<std::string>>(
                                           std::vector<std::string>{"cat", "dog"}));
  }

  CertificatePtr LoadIdentity(const std::string &privkey)
  {
    using Signer = fetch::crypto::ECDSASigner;
    // load the key
    auto signer = std::make_unique<Signer>();
    signer->Load(fetch::byte_array::FromBase64(privkey));
    return signer;
  }
};

class MuddleTypedUpdatesTests : public ::testing::Test
{
public:
  using Inst                   = LNP;
  using Insts                  = std::vector<Inst>;
  using MuddleLearnerNetworker = LN;

  std::shared_ptr<LearnerTypedUpdates> server;
  std::shared_ptr<LearnerTypedUpdates> client;
  unsigned short int                   server_port = 1766;
  unsigned short int                   client_port = 1767;

  void SetUp() override
  {
    srand(static_cast<unsigned int>(time(nullptr)));
    server_port = static_cast<unsigned short int>(rand() % 10000 + 10000);
    client_port = static_cast<unsigned short int>(server_port + 1);

    usleep(100000);
    server = std::make_shared<LearnerTypedUpdates>(SERVER_PUB, SERVER_PRIV, server_port, 0);
    usleep(100000);
    client =
        std::make_shared<LearnerTypedUpdates>(CLIENT_PUB, CLIENT_PRIV, client_port, server_port);
    usleep(100000);
  }
};

TEST_F(MuddleTypedUpdatesTests, singleThreadedVersion)
{
  sleep(1);
  server->actual->addTarget(client->pub_);

  server->PretendToLearn();

  sleep(1);
  EXPECT_GT(client->actual->GetUpdateTypeCount("update"), 0);
  EXPECT_GT(client->actual->GetUpdateTypeCount<UpdateTypeForTesting>(), 0);
  EXPECT_EQ(client->actual->GetUpdateTypeCount<UpdateTypeForTesting>(),
            client->actual->GetUpdateTypeCount("update"));
  EXPECT_GT(client->actual->GetUpdateTypeCount("vocab"), 0);
  /*
  try
  {
    client->actual->GetUpdateTypeCount("weights");
    EXPECT_NE(1, 1);
  }
  catch (std::exception &e)
  {
    EXPECT_EQ(1, 1);
  }

  try
  {
    client->actual->GetUpdateTypeCount<fetch::dmlf::Update<double>>();
    EXPECT_NE(1, 1);
  }
  catch (std::exception &e)
  {
    EXPECT_EQ(1, 1);
  }
  */
}

}  // namespace
