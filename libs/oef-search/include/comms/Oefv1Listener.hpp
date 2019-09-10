#pragma once

#include <memory>
#include <iostream>
#include <unordered_map>

#include "base/src/cpp/comms/IOefListener.hpp"
#include "base/src/cpp/comms/Listener.hpp"

class Core;
class SearchTaskFactory;
class OefSearchEndpoint;

template <template <typename> class EndpointType>
class Oefv1Listener:public IOefListener<SearchTaskFactory, OefSearchEndpoint>
{
public:
  using ConfigMap = std::unordered_map<std::string, std::string>;

  static constexpr char const *LOGGING_NAME = "Oefv1Listener";

  Oefv1Listener(std::shared_ptr<Core> core, int port, ConfigMap endpointConfig);
  virtual ~Oefv1Listener()
  {
    std::cout << "Listener on "<< port << " GONE" << std::endl;
  }

  void start(void);
protected:
private:
  Listener listener;
  int port;

  ConfigMap endpointConfig;

  Oefv1Listener(const Oefv1Listener &other) = delete;
  Oefv1Listener &operator=(const Oefv1Listener &other) = delete;
  bool operator==(const Oefv1Listener &other) = delete;
  bool operator<(const Oefv1Listener &other) = delete;
};
