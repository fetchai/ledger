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

using fetch::dmlf::IUpdate;
using fetch::dmlf::Update;

using namespace std::chrono_literals;

}  // namespace

class FakeLearner : public fetch::dmlf::AbstractLearnerNetworker
{
public:

  virtual void        pushUpdate(std::shared_ptr<IUpdate> update) override
  {
   auto msg = update->serialise();
   for (auto& peer : peers)
   {
      SendMessage(msg, peer);
   }
  }
  virtual std::size_t getPeerCount() const  override
  {
    return peers.size();
  }

  void AddPeer(std::shared_ptr<FakeLearner> peer)
  {
    peers.push_back(peer);
    peer->peers.push_back(std::shared_ptr<FakeLearner>{this});
  }

private:
  using Bytes = AbstractLearnerNetworker::Bytes;

  std::vector<std::shared_ptr<FakeLearner>> peers;

  void SendMessage(Bytes msg,  std::shared_ptr<FakeLearner>& peer)
  {
    peer->ReceiveMessage(msg);
  }

  void ReceiveMessage(Bytes msg)
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
  
  int                      num_upds = 10;
  std::shared_ptr<IUpdate> upd;
  FETCH_LOG_INFO(LOGGING_NAME, "Updates to push:");
  for (int i = 0; i < num_upds; i++)
  {
    auto upd = std::make_shared<Update<std::string>>(std::vector<std::string>{std::to_string(i)});
    learner1->pushUpdate(upd);
    FETCH_LOG_INFO(LOGGING_NAME, "Update pushed ", i, " ", upd->TimeStamp());
    std::this_thread::sleep_for(1.123456789s);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "[learner2] Updates from FakeLearner:");
  while (learner2->getUpdateCount())
  {
    upd = learner2->getUpdate<Update<std::string>>();
    FETCH_LOG_INFO(LOGGING_NAME, "Update received ", upd->TimeStamp());
  }

  FETCH_LOG_INFO(LOGGING_NAME, "[learner3] Updates from FakeLearner:");
  
  while (learner3->getUpdateCount())
  {
    upd = learner3->getUpdate<Update<std::string>>();
    FETCH_LOG_INFO(LOGGING_NAME, "Update received ", upd->TimeStamp());
  }

  return EXIT_SUCCESS;
}
