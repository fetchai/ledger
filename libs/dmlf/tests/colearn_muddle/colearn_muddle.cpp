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
#include "dmlf/colearn/update_store.hpp"
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
using Store          = fetch::dmlf::colearn::UpdateStore;
using StorePtr       = std::shared_ptr<Store>;

using UpdateTypeForTesting = fetch::dmlf::Update<TensorType>;
using UpdatePayload        = UpdateTypeForTesting::Payload;

char const *SERVER_PRIV = "BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=";
char const *SERVER_PUB =
    "rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==";
char const *CLIENT_PRIV = "4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=";
char const *CLIENT_PUB =
    "646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==";

char const *LOGGING_NAME = "colearn_muddle";

class LearnerTypedUpdates
{
public:
  LNP         actual;
  LNBaseP     interface;
  NetManP     netm_;
  MuddlePtr   mud_;
  std::string pub_;
  StorePtr    store_;

  LearnerTypedUpdates(const std::string &pub, const std::string &priv, unsigned short int port,
                      unsigned short int remote = 0)
  {
    pub_ = pub;

    std::string r;
    if (remote != 0)
    {
      r = "tcp://127.0.0.1:";
      r += std::to_string(remote);
    }

    actual    = std::make_shared<LN>(priv, port, r);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    interface = actual;
    interface->RegisterUpdateType<UpdateTypeForTesting>("update");
    interface->RegisterUpdateType<fetch::dmlf::Update<std::string>>("vocab");
  }

  void PretendToLearn()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Pretend Learning");
    static int sequence_number = 1;
    auto       t               = TensorType(TensorType::SizeType(2));
    t.Fill(DataType(sequence_number++));
    auto r = std::vector<TensorType>();
    r.push_back(t);
    interface->PushUpdateType("update", std::make_shared<UpdateTypeForTesting>(r));
    interface->PushUpdateType("vocab", std::make_shared<fetch::dmlf::Update<std::string>>(
                                           std::vector<std::string>{"cat", "dog"}));
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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server = std::make_shared<LearnerTypedUpdates>(SERVER_PUB, SERVER_PRIV, server_port, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client =
        std::make_shared<LearnerTypedUpdates>(CLIENT_PUB, CLIENT_PRIV, client_port, server_port);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
};

TEST_F(MuddleTypedUpdatesTests, singleThreadedVersion)
{
  sleep(1);
  server->actual->addTarget(client->pub_);

  server->PretendToLearn();

  sleep(1);
  EXPECT_GT(client->actual->GetUpdateCount(), 0);

  try
  {
    client->actual->GetUpdate("algo1", "vocab");
  }
  catch (std::exception &e)
  {
    EXPECT_EQ("vocab", "1 should be present");
  }

  try
  {
    client->actual->GetUpdate("algo1", "weights");
    EXPECT_EQ("weights", "should not be present");
  }
  catch (std::exception &e)
  {
    // get should throw, because weights Q is empty.
  }

  try
  {
    auto upd = client->actual->GetUpdate("algo1", "vocab");
    EXPECT_EQ("vocab", "should not be present (already emptied)");
  }
  catch (std::exception &e)
  {
    // get should throw, because vocab Q is empty.
  }
}

}  // namespace
