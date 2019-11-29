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

#include "dmlf/deprecated/filepassing_learner_networker.hpp"
#include "dmlf/deprecated/local_learner_networker.hpp"
#include "dmlf/deprecated/muddle_learner_networker.hpp"
#include "dmlf/deprecated/update.hpp"
#include "dmlf/deprecated/update_interface.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"

#include <chrono>
#include <ostream>
#include <thread>
#include <unordered_set>

namespace {

using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;

using LNBase  = fetch::dmlf::deprecated_AbstractLearnerNetworker;
using LN      = fetch::dmlf::deprecated_MuddleLearnerNetworker;
using LNBaseP = std::shared_ptr<LNBase>;
using LNP     = std::shared_ptr<LN>;

using UpdateTypeForTesting = fetch::dmlf::deprecated_Update<TensorType>;
using UpdatePayload        = UpdateTypeForTesting::Payload;

class Learner
{
public:
  std::shared_ptr<fetch::dmlf::deprecated_MuddleLearnerNetworker>   actual;
  std::shared_ptr<fetch::dmlf::deprecated_AbstractLearnerNetworker> interface;

  Learner(fetch::json::JSONDocument &cloud_config, std::size_t instance_number)
  {
    actual    = std::make_shared<fetch::dmlf::deprecated_MuddleLearnerNetworker>(cloud_config,
                                                                              instance_number);
    interface = actual;
    interface->Initialize<UpdateTypeForTesting>();
  }

  void PretendToLearn()
  {
    static int sequence_number = 1;
    auto       t               = TensorType(TensorType::SizeType(2));
    t.Fill(DataType(sequence_number++));
    auto r = std::vector<TensorType>();
    r.push_back(t);
    interface->PushUpdate(std::make_shared<UpdateTypeForTesting>(r));
  }
};

class deprecated_MuddleLearnerNetworkerTests : public ::testing::Test
{
public:
  using Inst                              = LNP;
  using Insts                             = std::vector<Inst>;
  using deprecated_MuddleLearnerNetworker = fetch::dmlf::deprecated_MuddleLearnerNetworker;

  Insts insts;

  std::vector<std::shared_ptr<Learner>> learners;

  fetch::json::JSONDocument json_config;

  void SetUp() override
  {
    std::string json_config_input = std::string("{") + "  \"peers\": [ " +
                                    "  { \"uri\": \"tcp://127.0.0.1:8000\", \"key\": "
                                    "\"BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=\", \"pub\": "
                                    "\"rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/"
                                    "Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==\" }, " +
                                    "  { \"uri\": \"tcp://127.0.0.1:8001\", \"key\": "
                                    "\"4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=\",  \"pub\": "
                                    "\"646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/"
                                    "PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==\" } " +
                                    "]" + "}";

    json_config.Parse(json_config_input.c_str());

    for (std::size_t i = 0; i < 2; i++)
    {
      learners.push_back(std::make_shared<Learner>(json_config, i));
    }
  }
};

TEST_F(deprecated_MuddleLearnerNetworkerTests, singleThreadedVersion)
{
  sleep(1);
  learners[0]->PretendToLearn();

  sleep(1);
  EXPECT_GT(learners[1]->actual->GetUpdateCount(), 0);
}

class LearnerTypedUpdates
{
public:
  std::shared_ptr<fetch::dmlf::deprecated_MuddleLearnerNetworker>   actual;
  std::shared_ptr<fetch::dmlf::deprecated_AbstractLearnerNetworker> interface;

  LearnerTypedUpdates(fetch::json::JSONDocument &cloud_config, std::size_t instance_number)
  {
    actual = std::make_shared<fetch::dmlf::deprecated_MuddleLearnerNetworker>(
        cloud_config, instance_number, std::shared_ptr<fetch::network::NetworkManager>{},
        fetch::dmlf::MuddleChannel::MULTIPLEX);
    interface = actual;
    interface->RegisterUpdateType<UpdateTypeForTesting>("update");
    interface->RegisterUpdateType<fetch::dmlf::deprecated_Update<std::string>>("vocab");
  }

  void PretendToLearn()
  {
    static int sequence_number = 1;
    auto       t               = TensorType(TensorType::SizeType(2));
    t.Fill(DataType(sequence_number++));
    auto r = std::vector<TensorType>();
    r.push_back(t);
    interface->PushUpdateType("update", std::make_shared<UpdateTypeForTesting>(r));
    interface->PushUpdateType("vocab",
                              std::make_shared<fetch::dmlf::deprecated_Update<std::string>>(
                                  std::vector<std::string>{"cat", "dog"}));
  }
};

class MuddleTypedUpdatesTests : public ::testing::Test
{
public:
  using Inst                              = LNP;
  using Insts                             = std::vector<Inst>;
  using deprecated_MuddleLearnerNetworker = fetch::dmlf::deprecated_MuddleLearnerNetworker;

  Insts insts;

  std::vector<std::shared_ptr<LearnerTypedUpdates>> learners;

  fetch::json::JSONDocument json_config;

  void SetUp() override
  {
    std::string json_config_input = std::string("{") + "  \"peers\": [ " +
                                    "  { \"uri\": \"tcp://127.0.0.1:8000\", \"key\": "
                                    "\"BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=\", \"pub\": "
                                    "\"rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/"
                                    "Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==\" }, " +
                                    "  { \"uri\": \"tcp://127.0.0.1:8001\", \"key\": "
                                    "\"4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=\",  \"pub\": "
                                    "\"646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/"
                                    "PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==\" } " +
                                    "]" + "}";

    json_config.Parse(json_config_input.c_str());

    for (std::size_t i = 0; i < 2; i++)
    {
      learners.push_back(std::make_shared<LearnerTypedUpdates>(json_config, i));
    }
  }
};

TEST_F(MuddleTypedUpdatesTests, singleThreadedVersion)
{
  sleep(1);
  learners[0]->PretendToLearn();

  sleep(1);
  EXPECT_GT(learners[1]->actual->GetUpdateTypeCount("update"), 0);
  EXPECT_GT(learners[1]->actual->GetUpdateTypeCount<UpdateTypeForTesting>(), 0);
  EXPECT_EQ(learners[1]->actual->GetUpdateTypeCount<UpdateTypeForTesting>(),
            learners[1]->actual->GetUpdateTypeCount("update"));
  EXPECT_GT(learners[1]->actual->GetUpdateTypeCount("vocab"), 0);

  try
  {
    learners[1]->actual->GetUpdateTypeCount("weights");
    EXPECT_NE(1, 1);
  }
  catch (std::exception const &e)
  {
    EXPECT_EQ(1, 1);
  }

  try
  {
    learners[1]->actual->GetUpdateTypeCount<fetch::dmlf::deprecated_Update<double>>();
    EXPECT_NE(1, 1);
  }
  catch (std::exception const &e)
  {
    EXPECT_EQ(1, 1);
  }
}

}  // namespace
