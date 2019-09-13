#include <iostream>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>

#include "dmlf/update.hpp"
#include "dmlf/muddle_learner_networker.hpp"
#include "core/logging.hpp"

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
  
  if(argc == 3)
  {
    fetch::network::Peer peer{argv[1], static_cast<uint16_t>(atoi(argv[2]))};
    uris.push_back(fetch::network::Uri{peer});
  }

  fetch::dmlf::MuddleLearnerNetworker muddle_learner{uris};
  FETCH_LOG_INFO(LOGGING_NAME, "Proceeding to start ...");

  /*
  int num_upds = 10;
  std::shared_ptr<IUpdate> upd;
  FETCH_LOG_INFO(LOGGING_NAME, "Updates to push:");
  for (int i = 0 ; i < num_upds ; i++) 
  {
    auto upd = std::make_shared<Update<std::string>>(std::vector<std::string>{std::to_string(i)});
    muddle_learner.pushUpdate(upd);
    FETCH_LOG_INFO(LOGGING_NAME, "Update", i, " ", upd->TimeStamp());
    std::this_thread::sleep_for(0.654321s);
  }
  
  FETCH_LOG_INFO(LOGGING_NAME, "Updates from MuddleLearner:");
  while((upd = muddle_learner.getUpdate()) != nullptr) 
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Update ", upd->TimeStamp());
  }
  */

  muddle_learner.start();
  
  while(true)
  {
    std::this_thread::sleep_for(1s);
  }

  return EXIT_SUCCESS;
}


