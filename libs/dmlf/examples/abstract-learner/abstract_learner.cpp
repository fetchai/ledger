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

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "core/logging.hpp"
#include "dmlf/abstract_learner_networker.hpp"
#include "dmlf/update.hpp"

namespace {

constexpr char const *LOGGING_NAME = "main";

using fetch::dmlf::UpdateInterface;
using fetch::dmlf::Update;

using namespace std::chrono_literals;

}  // namespace

class FakeLearner : public fetch::dmlf::AbstractLearnerNetworker
{
public:
  void PushUpdate(const std::shared_ptr<UpdateInterface> &update) override
  {
    auto msg = update->Serialise();
    for (auto &peer : peers)
    {
      SendMessage(msg, peer);
    }
  }
  std::size_t GetPeerCount() const override
  {
    return peers.size();
  }

  void AddPeer(std::shared_ptr<FakeLearner> const &peer)
  {
    peers.push_back(peer);
    peer->peers.emplace_back(std::shared_ptr<FakeLearner>{this});
  }

private:
  using Bytes = AbstractLearnerNetworker::Bytes;
  using Peer  = std::shared_ptr<FakeLearner>;
  using Peers = std::vector<Peer>;

  Peers peers;

  void SendMessage(const Bytes &msg, const std::shared_ptr<FakeLearner> &peer)
  {
    peer->ReceiveMessage(msg);
  }

  void ReceiveMessage(const Bytes &msg)
  {
    AbstractLearnerNetworker::NewMessage(msg);
  }
};

int main(int /*argc*/, char ** /*argv*/)
{
  std::shared_ptr<FakeLearner> learner1 = std::make_shared<FakeLearner>();
  std::shared_ptr<FakeLearner> learner2 = std::make_shared<FakeLearner>();
  std::shared_ptr<FakeLearner> learner3 = std::make_shared<FakeLearner>();
  learner1->AddPeer(learner2);
  learner1->AddPeer(learner3);

  FETCH_LOG_INFO(LOGGING_NAME, "Proceeding to start ...");

  learner1->Initialize<Update<std::string>>();
  learner2->Initialize<Update<std::string>>();
  learner3->Initialize<Update<std::string>>();

  int                              num_upds = 10;
  std::shared_ptr<UpdateInterface> upd;
  FETCH_LOG_INFO(LOGGING_NAME, "Updates to push:");
  for (int i = 0; i < num_upds; i++)
  {
    auto upd = std::make_shared<Update<std::string>>(std::vector<std::string>{std::to_string(i)});
    learner1->PushUpdate(upd);
    FETCH_LOG_INFO(LOGGING_NAME, "Update pushed ", i, " ", upd->TimeStamp());
    std::this_thread::sleep_for(1.123456789s);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "[learner2] Updates from FakeLearner:");
  while (learner2->GetUpdateCount() > 0)
  {
    upd = learner2->GetUpdate<Update<std::string>>();
    FETCH_LOG_INFO(LOGGING_NAME, "Update received ", upd->TimeStamp());
  }

  FETCH_LOG_INFO(LOGGING_NAME, "[learner3] Updates from FakeLearner:");

  while (learner3->GetUpdateCount() > 0)
  {
    upd = learner3->GetUpdate<Update<std::string>>();
    FETCH_LOG_INFO(LOGGING_NAME, "Update received ", upd->TimeStamp());
  }

  return EXIT_SUCCESS;
}
