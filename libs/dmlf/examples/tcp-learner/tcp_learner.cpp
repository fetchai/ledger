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
#include "dmlf/tcp_learner_networker.hpp"
#include "dmlf/update.hpp"

namespace {

constexpr char const *LOGGING_NAME = "main";

using fetch::dmlf::IUpdate;
using fetch::dmlf::Update;

using namespace std::chrono_literals;

}  // namespace

int main(int argc, char **argv)
{
  FETCH_LOG_INFO(LOGGING_NAME, "MAIN ", argc, " ", argv);

  std::vector<fetch::network::Uri> uris{};

  if (argc == 3)
  {
    fetch::network::Peer peer{argv[1], static_cast<uint16_t>(atoi(argv[2]))};
    uris.push_back(fetch::network::Uri{peer});
  }

  std::shared_ptr<fetch::dmlf::ILearnerNetworker> tcp_learner =
      std::make_shared<fetch::dmlf::TcpLearnerNetworker>(uris);
  FETCH_LOG_INFO(LOGGING_NAME, "Proceeding to start ...");

  // tcp_learner->start();
  std::this_thread::sleep_for(5s);

  int                      num_upds = 10;
  std::shared_ptr<IUpdate> upd;
  FETCH_LOG_INFO(LOGGING_NAME, "Updates to push:");
  for (int i = 0; i < num_upds; i++)
  {
    auto upd = std::make_shared<Update<std::string>>(std::vector<std::string>{std::to_string(i)});
    tcp_learner->pushUpdate(upd);
    FETCH_LOG_INFO(LOGGING_NAME, "Update pushed ", i, " ", upd->TimeStamp());
    std::this_thread::sleep_for(1.123456789s);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Updates from MuddleLearner:");
  while ((upd = tcp_learner->getUpdate<Update<std::string>>()) != nullptr)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Update received ", upd->TimeStamp());
  }

  while (true)
  {
    std::this_thread::sleep_for(1s);
  }

  return EXIT_SUCCESS;
}
