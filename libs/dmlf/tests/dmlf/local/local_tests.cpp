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
#include <thread>
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
    //using Mutex = fetch::Mutex;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    NetP net;
    std::size_t number;
    std::size_t integrations;
    std::size_t produced;
    mutable Mutex mut;
    bool quitflag = false;

    LocalLearnerInstance(NetP net, std::size_t number)
    {
      this -> integrations = 0;
      this -> number = number;
      this -> net = net;
      this -> produced = 0;
    }

    bool work(void)
    {
      bool result = false;
      while(produced < 10)
      {
        auto output = std::to_string(number) + ":" + std::to_string(produced);
        produced++;
        auto upd =std::make_shared<DummyUpdate>(output);
        net -> pushUpdate(upd);
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

    void quit(void)
    {
      Lock lock(mut);
      quitflag = true;
    }

    void mt_work(void)
    {
      while(true)
      {
        {
          Lock lock(mut);
          if (quitflag)
          {
            return;
          }
        }

        if (produced < 10)
        {
          auto output = std::to_string(number) + ":" + std::to_string(produced);
          produced++;
          auto upd =std::make_shared<DummyUpdate>(output);
          net -> pushUpdate(upd);
          continue;
        }

        if (net -> getUpdateCount() > 0)
        {
          net -> getUpdate();
          integrations++;
          continue;
        }

        sleep(1);
      }
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
      fetch::dmlf::LocalLearnerNetworker::resetAll();
    }

    void DoWork()
    {
      for(std::size_t i = 0;i< 20; i++)
      {
        auto local = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
        auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(20, 5);
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
    void DoMtWork()
    {
      for(std::size_t i = 0;i< 20; i++)
      {
        auto local = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
        auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(20, 5);
        std::shared_ptr<fetch::dmlf::ILearnerNetworker> interf = local;
        interf -> setShuffleAlgorithm(alg);
        insts.push_back(std::make_shared<LocalLearnerInstance>(interf, i));
      }

      using Thread = std::thread;
      using ThreadP = std::shared_ptr<Thread>;
      using Threads = std::list<ThreadP>;

      Threads threads;

      for (auto inst : insts)
      {
        auto func = [inst](){
          inst->mt_work();
        };

        auto t = std::make_shared<std::thread>(func);
        threads.push_back(t);
      }
      sleep(3);

      for (auto inst : insts)
      {
        inst->quit();
      }

      for (auto &t : threads)
      {
        t -> join();
      }
    }
  };

  TEST_F(LocalLearnerNetworkerTests, singleThreadedVersion)
  {
    DoWork();

    std::size_t total_integrations = 0;
    for(auto inst : insts)
    {
      total_integrations += inst -> integrations;
    }

    EXPECT_EQ(insts.size(),20);
    EXPECT_EQ(total_integrations, 20 * 10 * 5);
  }

  TEST_F(LocalLearnerNetworkerTests, multiThreadedVersion)
  {
    DoMtWork();

    std::size_t total_integrations = 0;
    for(auto inst : insts)
    {
      total_integrations += inst -> integrations;
    }

    EXPECT_EQ(insts.size(),20);
    EXPECT_EQ(total_integrations, 20 * 10 * 5);
    }
}  // namespace

