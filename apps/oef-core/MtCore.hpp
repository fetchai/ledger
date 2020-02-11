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
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-base/threading/Threadpool.hpp"
#include "oef-core/agents/Agents.hpp"
#include "oef-core/comms/OefListenerStarterTask.hpp"
#include "oef-core/comms/public_key_utils.hpp"

#include "oef-messages/config.hpp"

class Core;
class IKarmaPolicy;

class MtCore
{
public:
  static constexpr char const *LOGGING_NAME = "MtCore";

  MtCore()               = default;
  MtCore(MtCore const &) = delete;
  MtCore(MtCore &&)      = delete;
  ~MtCore()              = default;

  bool configure(const std::string &config_file = "", const std::string &config_json = "");
  int  run();

  MtCore &operator=(MtCore const &) = delete;
  MtCore &operator=(MtCore &&) = delete;

private:
  std::shared_ptr<IKarmaPolicy>                                                        karma_policy;
  std::shared_ptr<OefListenerSet<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint>> listeners;
  std::shared_ptr<Core>                                                                core;
  std::shared_ptr<fetch::oef::base::Taskpool>                                          tasks;
  std::shared_ptr<OutboundConversations>                                               outbounds;
  std::shared_ptr<Agents>                                                              agents_;
  fetch::oef::pb::CoreConfig                                                           config_;

  std::shared_ptr<std::set<PublicKey>> white_list_;
  bool                                 white_list_enabled_{false};

  fetch::oef::base::Threadpool comms_runners;
  fetch::oef::base::Threadpool tasks_runners;

  void startListeners(IKarmaPolicy *karmaPolicy);
  bool configureFromJsonFile(const std::string &config_file);
  bool configureFromJson(const std::string &config_json);
  bool load_ssl_pub_keys(std::string white_list_file);
};
