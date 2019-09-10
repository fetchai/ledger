#pragma once

#include "mt-core/tasks-base/src/cpp/IMtCoreTask.hpp"
#include "base/src/cpp/comms/IOefListener.hpp"
#include "base/src/cpp/comms/OefListenerSet.hpp"
#include "base/src/cpp/comms/Core.hpp"

#include <unordered_map>

class OefAgentEndpoint;
template <class OefEndpoint>
class IOefTaskFactory;
class IKarmaPolicy;

template <template <typename> class EndpointType>
class OefListenerStarterTask:public IMtCoreTask
{
public:
  using FactoryCreator = IOefListener<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint>::FactoryCreator;
  using ConfigMap      = std::unordered_map<std::string, std::string>;

  OefListenerStarterTask(int p,
                         std::shared_ptr< OefListenerSet<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint> > listeners,
                         std::shared_ptr<Core> core,
                         FactoryCreator initialFactoryCreator,
                         IKarmaPolicy *karmaPolicy,
                         ConfigMap endpointConfig)
  {
    this -> p = p;
    this -> listeners = listeners;
    this -> core = core;
    this -> initialFactoryCreator = initialFactoryCreator;
    this -> karmaPolicy = karmaPolicy;
    this -> endpointConfig = std::move(endpointConfig);
  }
  virtual ~OefListenerStarterTask()
  {
  }

  virtual bool isRunnable(void) const
  {
    return true;
  }
  virtual ExitState run(void);
protected:
private:
  std::shared_ptr<OefListenerSet<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint>> listeners;
  std::shared_ptr<Core> core;
  int p;
  FactoryCreator initialFactoryCreator;
  IKarmaPolicy *karmaPolicy;
  ConfigMap endpointConfig;

  OefListenerStarterTask(const OefListenerStarterTask &other) = delete;
  OefListenerStarterTask &operator=(const OefListenerStarterTask &other) = delete;
  bool operator==(const OefListenerStarterTask &other) = delete;
  bool operator<(const OefListenerStarterTask &other) = delete;
};
