#pragma once

#include <memory>
#include <iostream>
#include <unordered_map>

#include "base/src/cpp/comms/IOefListener.hpp"
#include "base/src/cpp/comms/Listener.hpp"

class Core;
class OefAgentEndpoint;
template <class OefEndpoint>
class IOefTaskFactory;
class IKarmaPolicy;

template <template <typename> class EndpointType>
class Oefv1Listener:public IOefListener<IOefTaskFactory<OefAgentEndpoint>, OefAgentEndpoint>
{
public:
  using ConfigMap = std::unordered_map<std::string, std::string>;

  Oefv1Listener(std::shared_ptr<Core> core, int port, IKarmaPolicy *karmaPolicy, ConfigMap endpointConfig);
  virtual ~Oefv1Listener()
  {
    std::cout << "Listener on "<< port << " GONE" << std::endl;
  }

  void start(void);
protected:
private:
  Listener listener;
  int port;

  IKarmaPolicy *karmaPolicy;
  ConfigMap endpointConfig;

  Oefv1Listener(const Oefv1Listener &other) = delete;
  Oefv1Listener &operator=(const Oefv1Listener &other) = delete;
  bool operator==(const Oefv1Listener &other) = delete;
  bool operator<(const Oefv1Listener &other) = delete;
};
