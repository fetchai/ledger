#pragma once

#include <vector>
#include <string>
#include <set>

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-base/threading/Threadpool.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "logging/logging.hpp"

#include "oef-messages/search_config.hpp"
#include "oef-search/functions/SearchTaskFactory.hpp"
#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/dap_manager/DapManager.hpp"
#include "oef-search/search_comms/SearchPeerStore.hpp"

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
  std::shared_ptr<OefListenerSet<IOefTaskFactory<OefSearchEndpoint>, OefSearchEndpoint>> listeners;
  std::shared_ptr<OutboundConversations> outbounds;
  std::shared_ptr<DapStore> dap_store_;
  std::shared_ptr<DapManager> dap_manager_;
  std::shared_ptr<SearchPeerStore> search_peer_store_;
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
