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

#include "dmlf/iupdate.hpp"
#include "dmlf/local_learner_networker.hpp"
#include "dmlf/filepassing_learner_networker.hpp"
#include "dmlf/muddle2_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "dmlf/update.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"

#include <chrono>
#include <ostream>
#include <thread>
#include <unordered_set>

namespace {

using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;

using LNBase     = fetch::dmlf::ILearnerNetworker;
using LN         = fetch::dmlf::Muddle2LearnerNetworker;
using LNBaseP    = std::shared_ptr<LNBase>;
using LNP        = std::shared_ptr<LN>;

using UpdateTypeForTesting = fetch::dmlf::Update<TensorType>;
using UpdatePayloadType    = UpdateTypeForTesting::PayloadType;

//static constexpr char const *NETWORK_A_PUBLIC_KEY =
//    "rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==";
//static constexpr char const *NETWORK_A_PRIVATE_KEY =
  //    "BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=";

//static constexpr char const *NETWORK_B_PUBLIC_KEY =
//    "646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==";
//static constexpr char const *NETWORK_B_PRIVATE_KEY =
//    "4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=";

class Learner
  {
  public:
    std::shared_ptr<fetch::dmlf::Muddle2LearnerNetworker> actual;
    std::shared_ptr<fetch::dmlf::ILearnerNetworker> interface;

    Learner(const std::string cloud_config,
            std::size_t instance_number)
    {
      actual = std::make_shared<fetch::dmlf::Muddle2LearnerNetworker>(cloud_config, instance_number);
      interface = actual;
    }

    void PretendToLearn()
    {
      static int sequence_number = 1;
      auto t = TensorType(TensorType::SizeType(2));
      t.Fill(DataType(sequence_number++));
      auto r = std::vector<TensorType>();
      r.push_back(t);
      interface -> pushUpdate(std::make_shared<UpdateTypeForTesting>(r));
    }

  };

class Muddle2LearnerNetworkerTests : public ::testing::Test
{
public:
  using Inst  = LNP;
  using Insts = std::vector<Inst>;
  using Muddle2LearnerNetworker = fetch::dmlf::Muddle2LearnerNetworker;

  Insts insts;

  std::vector<std::shared_ptr<Learner>> learners;

  std::string json_config = std::string("{")
    + "  \"peers\": [ "
    + "  { \"uri\": \"tcp://127.0.0.1:8000\", \"key\": \"BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=\", \"pub\": \"rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==\" }, "
    + "  { \"uri\": \"tcp://127.0.0.1:8001\", \"key\": \"4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=\",  \"pub\": \"646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==\" } "
    + "]"
    + "}";

  void SetUp() override
  {
    for(std::size_t i = 0; i < 2; i++)
    {
      learners.push_back(std::make_shared<Learner>(json_config, i));
    }
  }

};

TEST_F(Muddle2LearnerNetworkerTests, singleThreadedVersion)
{
  sleep(1);
  learners[0] -> PretendToLearn();

  sleep(1);
  EXPECT_GT(learners[1] -> actual -> getUpdateCount(), 0);
}



}  // namespace
