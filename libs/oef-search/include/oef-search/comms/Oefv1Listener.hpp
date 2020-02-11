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

  Oefv1Listener(std::shared_ptr<Core> const &core, unsigned short int port,
                ConfigMap endpointConfig);
  virtual ~Oefv1Listener()
  {
    std::cout << "Listener on " << port << " GONE" << std::endl;
  }

  void start();

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
