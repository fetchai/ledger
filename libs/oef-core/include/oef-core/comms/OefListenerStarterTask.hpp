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

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/IOefListener.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-core/tasks-base/IMtCoreTask.hpp"

#include <unordered_map>

class OefAgentEndpoint;
template <class OefEndpoint>
class IOefTaskFactory;
class IKarmaPolicy;

template <template <typename> class EndpointType>
class OefListenerStarterTask : public IMtCoreTask
{
public:
  using FactoryCreator =
      IOefListener<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint>::FactoryCreator;
  using ConfigMap = std::unordered_map<std::string, std::string>;

  OefListenerStarterTask(
      int p,
      std::shared_ptr<OefListenerSet<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint>>
                            listeners,
      std::shared_ptr<Core> core, FactoryCreator initialFactoryCreator, IKarmaPolicy *karmaPolicy,
      ConfigMap endpointConfig)
  {
    this->p                     = p;
    this->listeners             = listeners;
    this->core                  = core;
    this->initialFactoryCreator = initialFactoryCreator;
    this->karmaPolicy           = karmaPolicy;
    this->endpointConfig        = std::move(endpointConfig);
  }
  ~OefListenerStarterTask() override = default;

  bool IsRunnable() const override
  {
    return true;
  }
  fetch::oef::base::ExitState run() override;

protected:
private:
  std::shared_ptr<OefListenerSet<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint>> listeners;
  std::shared_ptr<Core>                                                                core;
  int                                                                                  p;
  FactoryCreator initialFactoryCreator;
  IKarmaPolicy * karmaPolicy;
  ConfigMap      endpointConfig;

  OefListenerStarterTask(const OefListenerStarterTask &other) = delete;
  OefListenerStarterTask &operator=(const OefListenerStarterTask &other)  = delete;
  bool                    operator==(const OefListenerStarterTask &other) = delete;
  bool                    operator<(const OefListenerStarterTask &other)  = delete;
};
