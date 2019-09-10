#pragma once

#include <vector>
#include <string>
#include <set>

#include "base/src/cpp/comms/Core.hpp"
#include "base/src/cpp/comms/OefListenerSet.hpp"
#include "base/src/cpp/conversation/OutboundConversations.hpp"
#include "base/src/cpp/threading/Taskpool.hpp"
#include "base/src/cpp/threading/Threadpool.hpp"
#include "fetch_teams/ledger/logger.hpp"

#include "protos/src/protos/search_config.pb.h"
#include "mt-search/comms/src/cpp/SearchTaskFactory.hpp"

class Core;
class OefSearchEndpoint;

class MtSearch
{
public:
  
  static constexpr char const *LOGGING_NAME = "MtSearch";

  MtSearch()
  {
  }
  virtual ~MtSearch()
  {
  }

  bool configure(const std::string &config_file="", const std::string &config_json="");
  int run();
protected:
private:
  std::shared_ptr<Core> core;
  std::shared_ptr<Taskpool> tasks;
  std::shared_ptr<OefListenerSet<SearchTaskFactory, OefSearchEndpoint>> listeners;
  std::shared_ptr<OutboundConversations> outbounds;
  fetch::oef::pb::SearchConfig config_;

  Threadpool comms_runners;
  Threadpool tasks_runners;

  void startListeners();
  bool configureFromJsonFile(const std::string &config_file);
  bool configureFromJson(const std::string &config_json);


  MtSearch(const MtSearch &other) = delete;
  MtSearch &operator=(const MtSearch &other) = delete;
  bool operator==(const MtSearch &other) = delete;
  bool operator<(const MtSearch &other) = delete;
};
