#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

#include "oef-base/comms/IOefListener.hpp"
#include "oef-base/comms/Listener.hpp"

class Core;
template <class Endpoint>
class IOefTaskFactory;
class OefSearchEndpoint;

template <template <typename> class EndpointType>
class Oefv1Listener : public IOefListener<IOefTaskFactory<OefSearchEndpoint>, OefSearchEndpoint>
{
public:
  using ConfigMap = std::unordered_map<std::string, std::string>;

  static constexpr char const *LOGGING_NAME = "Oefv1Listener";

  Oefv1Listener(std::shared_ptr<Core> core, unsigned short int port, ConfigMap endpointConfig);
  virtual ~Oefv1Listener()
  {
    std::cout << "Listener on " << port << " GONE" << std::endl;
  }

  void start(void);

protected:
private:
  Listener listener;
  int      port;

  ConfigMap endpointConfig;

  Oefv1Listener(const Oefv1Listener &other) = delete;
  Oefv1Listener &operator=(const Oefv1Listener &other)  = delete;
  bool           operator==(const Oefv1Listener &other) = delete;
  bool           operator<(const Oefv1Listener &other)  = delete;
};
