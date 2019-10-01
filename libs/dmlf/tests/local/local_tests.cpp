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

#include "dmlf/filepassing_learner_networker.hpp"
#include "dmlf/iupdate.hpp"
#include "dmlf/local_learner_networker.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "dmlf/update.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"

#include <chrono>
#include <ostream>
#include <thread>

namespace {

using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;
using NetP       = std::shared_ptr<fetch::dmlf::AbstractLearnerNetworker>;

using UpdateTypeForTesting = fetch::dmlf::Update<TensorType>;
using UpdatePayloadType    = UpdateTypeForTesting::PayloadType;

class LocalLearnerInstance
{
public:
  // using Mutex = fetch::Mutex;
  using Mutex = std::mutex;
  using Lock  = std::unique_lock<Mutex>;

  NetP          net;
  std::size_t   number;
  std::size_t   integrations;
  std::size_t   produced;
  mutable Mutex mut;
  bool          quitflag = false;

  LocalLearnerInstance(NetP net, std::size_t number)
  {
    this->integrations = 0;
    this->number       = number;
    this->net          = std::move(net);
    this->produced     = 0;
    this->net->Initialize<UpdateTypeForTesting>();
  }

  std::vector<TensorType> generateFakeWorkOutput(std::size_t instance_number,
                                                 std::size_t sequence_number)
  {
    auto t = TensorType(TensorType::SizeType(instance_number + 2));
    t.Fill(DataType(sequence_number));
    auto r = std::vector<TensorType>(1);
    r.push_back(t);
    return r;
  }

  bool work(void)
  {
    bool result = false;
    while (produced < 10)
    {
      produced++;
      auto output = generateFakeWorkOutput(number, produced);
      auto upd    = std::make_shared<UpdateTypeForTesting>(output);
      net->pushUpdate(upd);
      result = true;
    }

    while (net->getUpdateCount() > 0)
    {
      net->getUpdate<UpdateTypeForTesting>();
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
    while (true)
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
        produced++;
        auto output = generateFakeWorkOutput(number, produced);
        auto upd    = std::make_shared<UpdateTypeForTesting>(output);
        net->pushUpdate(upd);
        continue;
      }

      if (net->getUpdateCount() > 0)
      {
        net->getUpdate<UpdateTypeForTesting>();
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
  using Inst  = std::shared_ptr<LocalLearnerInstance>;
  using Insts = std::vector<Inst>;

  Insts insts;

  void SetUp() override
  {}

  void DoWork()
  {
    const std::size_t                         peercount = 20;
    fetch::dmlf::LocalLearnerNetworker::Peers peers;
    for (std::size_t i = 0; i < peercount; i++)
    {
      auto local = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
      peers.push_back(local);
      std::shared_ptr<fetch::dmlf::AbstractLearnerNetworker> interf = local;
      insts.push_back(std::make_shared<LocalLearnerInstance>(interf, i));
    }
    for (const auto &peer : peers)
    {
      peer->addPeers(peers);
    }

    for (const auto &peer : peers)
    {
      auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(peer->getPeerCount(), 5);
      peer->setShuffleAlgorithm(alg);
    }

    bool working = true;
    while (working)
    {
      working = false;
      for (const auto &inst : insts)
      {
        if (inst->work())
        {
          working = true;
        }
      }
    }
  }
  void DoMtWork()
  {
    const std::size_t                         peercount = 20;
    fetch::dmlf::LocalLearnerNetworker::Peers peers;
    for (std::size_t i = 0; i < peercount; i++)
    {
      auto local = std::make_shared<fetch::dmlf::LocalLearnerNetworker>();
      peers.push_back(local);
      std::shared_ptr<fetch::dmlf::AbstractLearnerNetworker> interf = local;
      insts.push_back(std::make_shared<LocalLearnerInstance>(interf, i));
    }

    for (const auto &peer : peers)
    {
      peer->addPeers(peers);
    }

    for (const auto &peer : peers)
    {
      auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(peer->getPeerCount(), 5);
      peer->setShuffleAlgorithm(alg);
    }

    using Thread  = std::thread;
    using ThreadP = std::shared_ptr<Thread>;
    using Threads = std::list<ThreadP>;

    Threads threads;

    for (const auto &inst : insts)
    {
      auto func = [inst]() { inst->mt_work(); };

      auto t = std::make_shared<std::thread>(func);
      threads.push_back(t);
    }
    sleep(3);

    for (const auto &inst : insts)
    {
      inst->quit();
    }

    for (auto &t : threads)
    {
      t->join();
    }
  }
  void DoMtFilepassingWork()
  {
    const std::size_t                                                      peercount = 20;
    std::vector<std::shared_ptr<fetch::dmlf::FilepassingLearnerNetworker>> peers;
    fetch::dmlf::FilepassingLearnerNetworker::Peers                        names;

    for (std::size_t i = 0; i < peercount; i++)
    {
      std::string name = std::string("foo-") + std::to_string(i);
      auto        peer = std::make_shared<fetch::dmlf::FilepassingLearnerNetworker>();
      peer->setName(name);
      peers.push_back(peer);
      names.push_back(name);
      std::shared_ptr<fetch::dmlf::AbstractLearnerNetworker> interf = peer;
      insts.push_back(std::make_shared<LocalLearnerInstance>(interf, i));
    }

    for (const auto &peer : peers)
    {
      peer->addPeers(names);
    }

    for (const auto &peer : peers)
    {
      auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(peer->getPeerCount(), 5);
      peer->setShuffleAlgorithm(alg);
    }

    using Thread  = std::thread;
    using ThreadP = std::shared_ptr<Thread>;
    using Threads = std::list<ThreadP>;

    Threads threads;

    for (const auto & inst : insts)
    {
      auto func = [inst]() { inst->mt_work(); };

      auto t = std::make_shared<std::thread>(func);
      threads.push_back(t);
    }
    sleep(3);

    for (const auto & inst : insts)
    {
      inst->quit();
    }

    for (const auto &t : threads)
    {
      t->join();
    }
  }
};

TEST_F(LocalLearnerNetworkerTests, singleThreadedVersion)
{
  DoWork();

  std::size_t total_integrations = 0;
  for (const auto &inst : insts)
  {
    total_integrations += inst->integrations;
  }

  EXPECT_EQ(insts.size(), 20);
  EXPECT_EQ(total_integrations, 20 * 10 * 5);
}

TEST_F(LocalLearnerNetworkerTests, multiThreadedVersion)
{
  DoMtWork();

  std::size_t total_integrations = 0;
  for (const auto &inst : insts)
  {
    total_integrations += inst->integrations;
  }

  EXPECT_EQ(insts.size(), 20);
  EXPECT_EQ(total_integrations, 20 * 10 * 5);
}

TEST_F(LocalLearnerNetworkerTests, multiThreadedFilePassingVersion)
{
  DoMtFilepassingWork();

  std::size_t total_integrations = 0;
  for (const auto &inst : insts)
  {
    total_integrations += inst->integrations;
  }

  EXPECT_EQ(insts.size(), 20);
  EXPECT_EQ(total_integrations, 20 * 10 * 5);
}

using namespace std::chrono_literals;
class UpdateSerialisationTests : public ::testing::Test
{
protected:
  void SetUp() override
  {}
};

TEST_F(UpdateSerialisationTests, basicPass)
{
  std::shared_ptr<fetch::dmlf::IUpdate> update_1 =
      std::make_shared<fetch::dmlf::Update<int>>(std::vector<int>{1, 2, 4});
  std::this_thread::sleep_for(1.54321s);
  std::shared_ptr<fetch::dmlf::IUpdate> update_2 = std::make_shared<fetch::dmlf::Update<int>>();

  EXPECT_NE(update_1->TimeStamp(), update_2->TimeStamp());
  EXPECT_NE(update_1->Fingerprint(), update_2->Fingerprint());

  auto update_1_bytes = update_1->serialise();
  update_2->deserialise(update_1_bytes);

  EXPECT_EQ(update_1->TimeStamp(), update_2->TimeStamp());
  EXPECT_EQ(update_1->Fingerprint(), update_2->Fingerprint());
}

}  // namespace
