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

#include "core/mutex.hpp"
#include "dmlf/deprecated/filepassing_learner_networker.hpp"
#include "dmlf/deprecated/local_learner_networker.hpp"
#include "dmlf/deprecated/update.hpp"
#include "dmlf/deprecated/update_interface.hpp"
#include "dmlf/simple_cycling_algorithm.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor/tensor.hpp"

#include "gtest/gtest.h"

#include <chrono>
#include <thread>

namespace {

using Mutex      = fetch::Mutex;
using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;
using NetP       = std::shared_ptr<fetch::dmlf::deprecated_AbstractLearnerNetworker>;

using UpdateTypeForTesting = fetch::dmlf::deprecated_Update<TensorType>;
using UpdatePayload        = UpdateTypeForTesting::Payload;

class LocalLearnerInstance
{
public:
  using Lock = std::unique_lock<Mutex>;

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

  std::vector<TensorType> GenerateFakeWorkOutput(std::size_t instance_number,
                                                 std::size_t sequence_number)
  {
    auto t = TensorType(TensorType::SizeType(instance_number + 2));
    t.Fill(DataType(sequence_number));
    auto r = std::vector<TensorType>(1);
    r.push_back(t);
    return r;
  }

  bool Work()
  {
    bool result = false;
    while (produced < 10)
    {
      produced++;
      auto output = GenerateFakeWorkOutput(number, produced);
      auto upd    = std::make_shared<UpdateTypeForTesting>(output);
      net->PushUpdate(upd);
      result = true;
    }

    while (net->GetUpdateCount() > 0)
    {
      net->GetUpdate<UpdateTypeForTesting>();
      integrations++;
      result = true;
    }
    return result;
  }

  void quit()
  {
    FETCH_LOCK(mut);
    quitflag = true;
  }

  void MtWork()
  {
    while (true)
    {
      {
        FETCH_LOCK(mut);
        if (quitflag)
        {
          return;
        }
      }

      if (produced < 10)
      {
        produced++;
        auto output = GenerateFakeWorkOutput(number, produced);
        auto upd    = std::make_shared<UpdateTypeForTesting>(output);
        net->PushUpdate(upd);
        continue;
      }

      if (net->GetUpdateCount() > 0)
      {
        net->GetUpdate<UpdateTypeForTesting>();
        integrations++;
        continue;
      }

      sleep(1);
    }
  }
};

class deprecated_LocalLearnerNetworkerTests : public ::testing::Test
{
public:
  using Inst  = std::shared_ptr<LocalLearnerInstance>;
  using Insts = std::vector<Inst>;

  Insts insts;

  void SetUp() override
  {}

  void DoWork()
  {
    const std::size_t                                    peercount = 20;
    fetch::dmlf::deprecated_LocalLearnerNetworker::Peers peers;
    for (std::size_t i = 0; i < peercount; i++)
    {
      auto local = std::make_shared<fetch::dmlf::deprecated_LocalLearnerNetworker>();
      peers.push_back(local);
      std::shared_ptr<fetch::dmlf::deprecated_AbstractLearnerNetworker> interf = local;
      insts.push_back(std::make_shared<LocalLearnerInstance>(interf, i));
    }
    for (auto const &peer : peers)
    {
      peer->AddPeers(peers);
    }

    for (auto const &peer : peers)
    {
      auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(peer->GetPeerCount(), 5);
      peer->SetShuffleAlgorithm(alg);
    }

    bool working = true;
    while (working)
    {
      working = false;
      for (auto const &inst : insts)
      {
        if (inst->Work())
        {
          working = true;
        }
      }
    }
  }
  void DoMtWork()
  {
    const std::size_t                                    peercount = 20;
    fetch::dmlf::deprecated_LocalLearnerNetworker::Peers peers;
    for (std::size_t i = 0; i < peercount; i++)
    {
      auto local = std::make_shared<fetch::dmlf::deprecated_LocalLearnerNetworker>();
      peers.push_back(local);
      std::shared_ptr<fetch::dmlf::deprecated_AbstractLearnerNetworker> interf = local;
      insts.push_back(std::make_shared<LocalLearnerInstance>(interf, i));
    }

    for (auto const &peer : peers)
    {
      peer->AddPeers(peers);
    }

    for (auto const &peer : peers)
    {
      auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(peer->GetPeerCount(), 5);
      peer->SetShuffleAlgorithm(alg);
    }

    using Thread  = std::thread;
    using ThreadP = std::shared_ptr<Thread>;
    using Threads = std::list<ThreadP>;

    Threads threads;

    for (auto const &inst : insts)
    {
      auto func = [inst]() { inst->MtWork(); };

      auto t = std::make_shared<std::thread>(func);
      threads.push_back(t);
    }
    sleep(3);

    for (auto const &inst : insts)
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
    const std::size_t peercount = 20;
    std::vector<std::shared_ptr<fetch::dmlf::deprecated_FilepassingLearnerNetworker>> peers;
    fetch::dmlf::deprecated_FilepassingLearnerNetworker::Peers                        names;

    for (std::size_t i = 0; i < peercount; i++)
    {
      std::string name = std::string("foo-") + std::to_string(i);
      auto        peer = std::make_shared<fetch::dmlf::deprecated_FilepassingLearnerNetworker>();
      peer->SetName(name);
      peers.push_back(peer);
      names.push_back(name);
      std::shared_ptr<fetch::dmlf::deprecated_AbstractLearnerNetworker> interf = peer;
      insts.push_back(std::make_shared<LocalLearnerInstance>(interf, i));
    }

    for (auto const &peer : peers)
    {
      peer->AddPeers(names);
    }

    for (auto const &peer : peers)
    {
      auto alg = std::make_shared<fetch::dmlf::SimpleCyclingAlgorithm>(peer->GetPeerCount(), 5);
      peer->SetShuffleAlgorithm(alg);
    }

    using Thread  = std::thread;
    using ThreadP = std::shared_ptr<Thread>;
    using Threads = std::list<ThreadP>;

    Threads threads;

    for (auto const &inst : insts)
    {
      auto func = [inst]() { inst->MtWork(); };

      auto t = std::make_shared<std::thread>(func);
      threads.push_back(t);
    }
    sleep(3);

    for (auto const &inst : insts)
    {
      inst->quit();
    }

    for (auto const &t : threads)
    {
      t->join();
    }
  }
};

TEST_F(deprecated_LocalLearnerNetworkerTests, singleThreadedVersion)
{
  DoWork();

  std::size_t total_integrations = 0;
  for (auto const &inst : insts)
  {
    total_integrations += inst->integrations;
  }

  EXPECT_EQ(insts.size(), 20);
  EXPECT_EQ(total_integrations, 20 * 10 * 5);
}

TEST_F(deprecated_LocalLearnerNetworkerTests, multiThreadedVersion)
{
  DoMtWork();

  std::size_t total_integrations = 0;
  for (auto const &inst : insts)
  {
    total_integrations += inst->integrations;
  }

  EXPECT_EQ(insts.size(), 20);
  EXPECT_EQ(total_integrations, 20 * 10 * 5);
}

// TODO(1841) fix or delete flaky test
// TEST_F(deprecated_LocalLearnerNetworkerTests, multiThreadedFilePassingVersion)
//{
//  DoMtFilepassingWork();
//
//  std::size_t total_integrations = 0;
//  for (auto const &inst : insts)
//  {
//    total_integrations += inst->integrations;
//  }
//
//  EXPECT_EQ(insts.size(), 20);
//  EXPECT_EQ(total_integrations, 20 * 10 * 5);
//}

using namespace std::chrono_literals;
class UpdateSerialisationTests : public ::testing::Test
{
};

TEST_F(UpdateSerialisationTests, basicPass)
{
  std::shared_ptr<fetch::dmlf::UpdateInterface> update_1 =
      std::make_shared<fetch::dmlf::deprecated_Update<int>>(std::vector<int>{1, 2, 4});
  std::this_thread::sleep_for(1.54321s);
  std::shared_ptr<fetch::dmlf::UpdateInterface> update_2 =
      std::make_shared<fetch::dmlf::deprecated_Update<int>>();

  EXPECT_NE(update_1->TimeStamp(), update_2->TimeStamp());
  EXPECT_NE(update_1->GetFingerprint(), update_2->GetFingerprint());

  auto update_1_bytes = update_1->Serialise();
  update_2->DeSerialise(update_1_bytes);

  EXPECT_EQ(update_1->TimeStamp(), update_2->TimeStamp());
  EXPECT_EQ(update_1->GetFingerprint(), update_2->GetFingerprint());
}

}  // namespace
