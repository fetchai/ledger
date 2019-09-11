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
#include "dmlf/simple_cycling_algorithm.hpp"

#include <ostream>

namespace {

  using NetP = std::shared_ptr<fetch::dmlf::ILearnerNetworker>;

  class DummyUpdate:public fetch::dmlf::IUpdate
  {
  public:
    std::string s;
    TimeStampType stamp_;
    using TimeStampType    = fetch::dmlf::IUpdate::TimeStampType;

    DummyUpdate(const std::string &s)
      : stamp_(static_cast<TimeStampType>
               (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()))
    {
      this -> s = s;
    }
    virtual fetch::byte_array::ByteArray serialise()
    {
      fetch::byte_array::ByteArray req{s.c_str()};
      return req;
    }
    virtual void deserialise(fetch::byte_array::ByteArray&a)
    {
      s = std::string(a);
    }
    virtual TimeStampType TimeStamp() const
    {
      return stamp_;
    }
  };

  class LocalLearnerInstance
  {
  public:
    NetP net;
    std::size_t number;
    std::size_t integrations;
    bool produced;

    LocalLearnerInstance(NetP net, std::size_t number)
    {
      this -> integrations = 0;
      this -> number = number;
      this -> net = net;
      this -> produced = false;
    }

    bool work(void)
    {
      bool result = false;
      if (!produced)
      {
        for(std::size_t cycle = 0; cycle < 10 ;cycle++)
        {
          auto output = std::to_string(number) + ":" + std::to_string(cycle);
          auto upd =std::make_shared<DummyUpdate>(output);
          net -> pushUpdate(upd);
        }
        produced = true;
        result = true;
      }

      while(net -> getUpdateCount() > 0)
      {
        net -> getUpdate();
        integrations++;
        result = true;
      }
      return result;
    }
  };

  class LocalLearnerNetworkerTests : public ::testing::Test
  {
  public:
    using Inst = std::shared_ptr<LocalLearnerInstance>;
    using Insts = std::vector<Inst>;

    Insts insts;

    void SetUp() override
    {
    }

    void DoWork()
    {
      for(std::size_t i = 0;i< 20; i++)
      {
        auto local = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
        auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(20, 20);
        std::shared_ptr<fetch::dmlf::ILearnerNetworker> interf = local;
        interf -> setShuffleAlgorithm(alg);
        insts.push_back(std::make_shared<LocalLearnerInstance>(interf, i));
      }

      bool working = true;
      while(working)
      {
        working = false;
        for(auto inst : insts)
        {
          if (inst -> work())
          {
            working = true;
          }
        }
      }
    }
  };

  TEST_F(LocalLearnerNetworkerTests, basicPass)
  {
    DoWork();

    std::vector<std::size_t> icount;
    for(auto inst : insts)
    {
      icount.push_back(inst -> integrations);
      std::cout << "integrations = " << std::to_string(inst -> integrations) << std::endl;
    }

    EXPECT_EQ(insts.size(),20);
  }

}  // namespace

