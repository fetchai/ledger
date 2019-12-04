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
#include "dmlf/collective_learning/utilities/typed_update_adaptor.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
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

using LNBase   = fetch::dmlf::colearn::AbstractMessageController;
using LNBaseT  = fetch::dmlf::collective_learning::utilities::TypedUpdateAdaptor;
using LN       = fetch::dmlf::colearn::MuddleLearnerNetworkerImpl;
using LNBaseP  = std::shared_ptr<LNBase>;
using LNBaseTP = std::shared_ptr<LNBaseT>;
using LNP      = std::shared_ptr<LN>;
using NetMan   = fetch::network::NetworkManager;
using NetManP  = std::shared_ptr<NetMan>;

using MuddlePtr      = fetch::muddle::MuddlePtr;
using CertificatePtr = fetch::muddle::ProverPtr;
using Store          = fetch::dmlf::colearn::UpdateStore;
using StorePtr       = std::shared_ptr<Store>;

using UpdateTypeForTesting = fetch::dmlf::deprecated_Update<TensorType>;
using UpdatePayload        = UpdateTypeForTesting::Payload;

// char const *SERVER_PRIV = "BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=";
// char const *SERVER_PUB =
//    "rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==";
// char const *CLIENT_PRIV = "4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=";
// char const *CLIENT_PUB =
//    "646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==";

char const *LOGGING_NAME = "colearn_muddle";

class LearnerTypedUpdates
{
public:
  LNP       actual;
  LNBaseP   interface;
  LNBaseTP  interface_typed;
  NetManP   netm_;
  MuddlePtr mud_;
  StorePtr  store_;

  LearnerTypedUpdates(const std::string &priv, unsigned short int port,
                      unsigned short int remote = 0)
  {
    std::string r;
    if (remote != 0)
    {
      r = "tcp://127.0.0.1:";
      r += std::to_string(remote);
    }

    actual          = std::make_shared<LN>(priv, port, r);
    interface       = actual;
    interface_typed = std::make_shared<LNBaseT>(interface);
    interface_typed->RegisterUpdateType<UpdateTypeForTesting>("update");
    interface_typed->RegisterUpdateType<fetch::dmlf::deprecated_Update<std::string>>("vocab");
  }

  void PretendToLearn()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Pretend Learning");
    static int sequence_number = 1;
    auto       t               = TensorType(TensorType::SizeType(2));
    t.Fill(DataType(sequence_number++));
    auto r = std::vector<TensorType>();
    r.push_back(t);
    interface_typed->PushUpdate(std::make_shared<UpdateTypeForTesting>(r));
    interface_typed->PushUpdate(std::make_shared<fetch::dmlf::deprecated_Update<std::string>>(
        std::vector<std::string>{"cat", "dog"}));
  }
};

class MuddleTypedUpdatesTests : public ::testing::Test
{
public:
  using Inst = struct
  {
    std::shared_ptr<LearnerTypedUpdates> instance;
    unsigned short int                   port;
    LN::Address                          pub;
    std::string                          pubstr;
  };

  using Insts = std::vector<Inst>;

  Insts instances;

  void CreateServers(unsigned int peercount = 3)
  {
    srand(static_cast<unsigned int>(time(nullptr)));
    auto base_port = static_cast<unsigned short int>(rand() % 10000 + 10000);

    for (unsigned int i = 0; i < peercount; i++)
    {
      Inst inst;
      inst.port     = static_cast<unsigned short int>(base_port + i);
      inst.instance = std::make_shared<LearnerTypedUpdates>("", inst.port, (i > 0) ? base_port : 0);
      inst.pub      = inst.instance->actual->GetAddress();
      inst.pubstr   = inst.instance->actual->GetAddressAsString();
      instances.push_back(inst);
    }
  }
};

TEST_F(MuddleTypedUpdatesTests, correctMessagesArriveBCast)
{
  CreateServers(2);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  instances[0].instance->PretendToLearn();

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_GT(instances[1].instance->actual->GetUpdateCount(), 0);
  try
  {
    instances[1].instance->actual->GetUpdate("algo0", "vocab");
  }
  catch (std::exception const &e)
  {
    EXPECT_EQ("vocab", "1 should be present");
  }

  try
  {
    instances[1].instance->actual->GetUpdate("algo0", "weights");
    EXPECT_EQ("weights", "should not be present");
  }
  catch (std::exception const &e)
  {
    // get should throw, because weights Q is empty.
  }

  try
  {
    auto upd = instances[1].instance->actual->GetUpdate("algo0", "vocab");
    EXPECT_EQ("vocab", "should not be present (already emptied)");
  }
  catch (std::exception const &e)
  {
    // get should throw, because vocab Q is empty.
  }
}

TEST_F(MuddleTypedUpdatesTests, JSONhandling)
{
  fetch::json::JSONDocument json_config;
  std::string               json_config_input = std::string(
      "{"
      "    \"peers\": ["
      "        {"
      "            \"key\": \"mVnmrf9vsW1lzHvziA75wv1fjcRGToV9wm1Aa8FKlOM=\","
      "            \"pub\": "
      "\"5GBXgYsH6IBb6vP/"
      "xIagzgldgaFUhSNrkogEwI4nqYHFYEgNVXHnGcSExZzEQAYcyqf+E13TVwQkWN1EXO4njQ==\","
      "            \"uri\": \"tcp://127.0.0.1:8000\""
      "        },"
      "        {"
      "            \"key\": \"zli9+GFCsZpxvhYYLAvr2lroyCTuA1DUelO5ds4h+xE=\","
      "            \"pub\": "
      "\"bQhZKIzqsRr+vQji2961q41Sa/"
      "X3Zodjw7XXMP1PSzxFznBWKoYnYqyWDSRDmR9qQlRcr+777xxt5354VwuLOw==\","
      "            \"uri\": \"tcp://127.0.0.1:8001\""
      "        },"
      "        {"
      "            \"key\": \"7tEio6183tl+2k6zttJvUjXcfHhq0hcCnCzP0yuQyMQ=\","
      "            \"pub\": "
      "\"WLQDnuisKHTsQjSvyfU6wewWi8ABy1Wiup54MOPN+W5MppQAqZ6MQAuNrt1uHHAbLc+mLnUcFe+A8o3FpJz5/"
      "w==\","
      "            \"uri\": \"tcp://127.0.0.1:8002\""
      "        }"
      "    ]"
      "}");
  json_config.Parse(json_config_input.c_str());
  auto actual = std::make_shared<LN>(json_config, 2);
  EXPECT_EQ(actual->GetPeerCount(), 2);
}

TEST_F(MuddleTypedUpdatesTests, correctMessagesArriveShuffle)
{
  CreateServers(6);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  for (unsigned int i = 0; i < instances.size(); i++)
  {
    for (unsigned int j = 0; j < instances.size(); j++)
    {
      if (i != j)
      {
        instances[i].instance->actual->AddPeers({instances[j].pubstr});
      }
    }
  }

  auto cycle = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(instances.size() - 1, 2);
  for (auto &inst : instances)
  {
    inst.instance->actual->SetShuffleAlgorithm(cycle);
  }

  instances[0].instance->PretendToLearn();

  std::this_thread::sleep_for(std::chrono::milliseconds(700));

  EXPECT_EQ(instances[0].instance->actual->GetUpdateCount(), 0);
  EXPECT_EQ(instances[1].instance->actual->GetUpdateCount(), 1);
  EXPECT_EQ(instances[2].instance->actual->GetUpdateCount(), 1);
  EXPECT_EQ(instances[3].instance->actual->GetUpdateCount(), 1);
  EXPECT_EQ(instances[4].instance->actual->GetUpdateCount(), 1);
  EXPECT_EQ(instances[5].instance->actual->GetUpdateCount(), 0);
}

}  // namespace
