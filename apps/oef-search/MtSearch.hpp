#pragma once
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

#include <set>
#include <string>
#include <vector>

#include "logging/logging.hpp"
#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-base/threading/Threadpool.hpp"

#include "oef-messages/search_config.hpp"
#include "oef-search/comms/IAddSearchPeer.hpp"
#include "oef-search/dap_manager/DapManager.hpp"
#include "oef-search/dap_manager/DapStore.hpp"
#include "oef-search/functions/SearchTaskFactory.hpp"
#include "oef-search/search_comms/SearchPeerStore.hpp"

class Core;
class OefSearchEndpoint;

class MtSearch : public IAddSearchPeer, public std::enable_shared_from_this<MtSearch>
{
public:
  static constexpr char const *LOGGING_NAME = "MtSearch";

  MtSearch()           = default;
  ~MtSearch() override = default;

  bool configure(const std::string &config_file = "", const std::string &config_json = "");
  int  run();

  void AddPeer(const std::string &) override;

protected:
private:
  std::shared_ptr<Core>                                                                  core;
  std::shared_ptr<fetch::oef::base::Taskpool>                                            tasks;
  std::shared_ptr<OefListenerSet<IOefTaskFactory<OefSearchEndpoint>, OefSearchEndpoint>> listeners;
  std::shared_ptr<OutboundConversations>                                                 outbounds;
  std::shared_ptr<DapStore>                                                              dap_store_;
  std::shared_ptr<DapManager>      dap_manager_;
  std::shared_ptr<SearchPeerStore> search_peer_store_;
  fetch::oef::pb::SearchConfig     config_;

  fetch::oef::base::Threadpool comms_runners;
  fetch::oef::base::Threadpool tasks_runners;

  void startListeners();
  bool configureFromJsonFile(const std::string &config_file);
  bool configureFromJson(const std::string &config_json);

  MtSearch(const MtSearch &other) = delete;
  MtSearch &operator=(const MtSearch &other)  = delete;
  bool      operator==(const MtSearch &other) = delete;
  bool      operator<(const MtSearch &other)  = delete;
};
