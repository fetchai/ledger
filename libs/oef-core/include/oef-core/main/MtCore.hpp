#pragma once

#include <set>
#include <string>
#include <vector>

#include "core/logging.hpp"
#include "mt-core/agents/src/cpp/Agents.hpp"
#include "mt-core/comms/src/cpp/OefListenerStarterTask.hpp"
#include "mt-core/secure/experimental/cpp/public_key_utils.hpp"
#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/conversation/OutboundConversations.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-base/threading/Threadpool.hpp"

#include "protos/src/protos/config.pb.h"

class Core;
class IKarmaPolicy;

class MtCore
{
public:
  static constexpr char const *LOGGING_NAME = "MtCore";

  MtCore()
  {}
  virtual ~MtCore()
  {}

  bool configure(const std::string &config_file = "", const std::string &config_json = "");
  int  run();

protected:
private:
  std::shared_ptr<IKarmaPolicy>                                                        karma_policy;
  std::shared_ptr<OefListenerSet<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint>> listeners;
  std::shared_ptr<Core>                                                                core;
  std::shared_ptr<Taskpool>                                                            tasks;
  std::shared_ptr<OutboundConversations>                                               outbounds;
  std::shared_ptr<Agents>                                                              agents_;
  fetch::oef::pb::CoreConfig                                                           config_;

  std::shared_ptr<std::set<PublicKey>> white_list_;
  bool                                 white_list_enabled_;

  Threadpool comms_runners;
  Threadpool tasks_runners;

  void startListeners(IKarmaPolicy *karmaPolicy);
  bool configureFromJsonFile(const std::string &config_file);
  bool configureFromJson(const std::string &config_json);
  bool load_ssl_pub_keys(std::string white_list_file);

  MtCore(const MtCore &other) = delete;
  MtCore &operator=(const MtCore &other)  = delete;
  bool    operator==(const MtCore &other) = delete;
  bool    operator<(const MtCore &other)  = delete;
};
