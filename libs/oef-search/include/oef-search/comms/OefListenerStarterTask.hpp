#pragma once

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/IOefListener.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/threading/Task.hpp"

#include <unordered_map>

class OefSearchEndpoint;
template <class OefEndpoint>
class IOefTaskFactory;
class OefAgentEndpoint;

template <template <typename> class EndpointType>
class OefListenerStarterTask : public Task
{
public:
  using FactoryCreator =
      IOefListener<IOefTaskFactory<OefSearchEndpoint>, OefSearchEndpoint>::FactoryCreator;
  using ConfigMap = std::unordered_map<std::string, std::string>;

  static constexpr char const *LOGGING_NAME = "OefListenerStarterTask";

  OefListenerStarterTask(
      int p,
      std::shared_ptr<OefListenerSet<IOefTaskFactory<OefSearchEndpoint>, OefSearchEndpoint>>
                            listeners,
      std::shared_ptr<Core> core, FactoryCreator initialFactoryCreator, ConfigMap endpointConfig)
  {
    this->p                     = p;
    this->listeners             = listeners;
    this->core                  = core;
    this->initialFactoryCreator = initialFactoryCreator;
    this->endpointConfig        = std::move(endpointConfig);
  }
  virtual ~OefListenerStarterTask()
  {}

  virtual bool isRunnable(void) const
  {
    return true;
  }
  virtual ExitState run(void);

protected:
private:
  std::shared_ptr<OefListenerSet<IOefTaskFactory<OefSearchEndpoint>, OefSearchEndpoint>> listeners;
  std::shared_ptr<Core>                                                                  core;
  int                                                                                    p;
  FactoryCreator initialFactoryCreator;
  ConfigMap      endpointConfig;

  OefListenerStarterTask(const OefListenerStarterTask &other) = delete;
  OefListenerStarterTask &operator=(const OefListenerStarterTask &other)  = delete;
  bool                    operator==(const OefListenerStarterTask &other) = delete;
  bool                    operator<(const OefListenerStarterTask &other)  = delete;
};
