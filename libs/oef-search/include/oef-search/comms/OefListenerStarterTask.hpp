#pragma once
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

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/IOefListener.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/threading/Task.hpp"

#include <unordered_map>

class SearchTaskFactory;
class OefSearchEndpoint;
template <class OefEndpoint>
class IOefTaskFactory;
class OefAgentEndpoint;

template <template <typename> class EndpointType>
class OefListenerStarterTask : public Task
{
public:
  using FactoryCreator    = IOefListener<SearchTaskFactory, OefSearchEndpoint>::FactoryCreator;
  using ConfigMap         = std::unordered_map<std::string, std::string>;
  using SharedListenerSet = std::shared_ptr<OefListenerSet<SearchTaskFactory, OefSearchEndpoint>>;
  using SharedCore        = std::shared_ptr<Core>;

  static constexpr char const *LOGGING_NAME = "OefListenerStarterTask";

  /// @{
  OefListenerStarterTask(int p, SharedListenerSet listeners, SharedCore core,
                         FactoryCreator initialFactoryCreator, ConfigMap endpointConfig)
    : listeners_{listeners}
    , core_{core}
    , p_{p}
    , initialFactoryCreator_{initialFactoryCreator}
    , endpointConfig_{endpointConfig}
  {}

  OefListenerStarterTask(OefListenerStarterTask const &other) = delete;
  virtual ~OefListenerStarterTask()                           = default;
  /// @}

  /// @{
  OefListenerStarterTask &operator=(OefListenerStarterTask const &other)  = delete;
  bool                    operator==(OefListenerStarterTask const &other) = delete;
  bool                    operator<(OefListenerStarterTask const &other)  = delete;
  /// @}

  virtual bool IsRunnable(void) const
  {
    return true;
  }
  virtual ExitState run(void);

private:
  SharedListenerSet     listeners_;
  std::shared_ptr<Core> core_;
  int                   p_;  // TODO: rename variable
  FactoryCreator        initialFactoryCreator_;
  ConfigMap             endpointConfig_;
};
