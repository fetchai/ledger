#pragma once

#include <set>
#include <string>
#include <vector>

#include "core/logging.hpp"
#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-base/threading/Threadpool.hpp"

#include "mt-search/comms/src/cpp/SearchTaskFactory.hpp"
#include "protos/src/protos/search_config.pb.h"

class Core;
class OefSearchEndpoint;

class MtSearch
{
public:
  static constexpr char const *LOGGING_NAME = "MtSearch";

  MtSearch()
  {}
  virtual ~MtSearch()
  {}

  bool configure(const std::string &config_file = "", const std::string &config_json = "");
  int  run();

protected:
private:
  std::shared_ptr<Core>                                                 core;
  std::shared_ptr<Taskpool>                                             tasks;
  std::shared_ptr<OefListenerSet<SearchTaskFactory, OefSearchEndpoint>> listeners;
  std::shared_ptr<OutboundConversations>                                outbounds;
  fetch::oef::pb::SearchConfig                                          config_;

  Threadpool comms_runners;
  Threadpool tasks_runners;

  void startListeners();
  bool configureFromJsonFile(const std::string &config_file);
  bool configureFromJson(const std::string &config_json);

  MtSearch(const MtSearch &other) = delete;
  MtSearch &operator=(const MtSearch &other)  = delete;
  bool      operator==(const MtSearch &other) = delete;
  bool      operator<(const MtSearch &other)  = delete;
};
