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

#include "oef-core/comms/Oefv1Listener.hpp"

#include <memory>

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/EndpointWebSocket.hpp"
#include "oef-core/comms/EndpointSSL.hpp"
#include "oef-core/comms/OefAgentEndpoint.hpp"

template <template <typename> class EndpointType>
Oefv1Listener<EndpointType>::Oefv1Listener(std::shared_ptr<Core> const &core, unsigned short port,
                                           IKarmaPolicy *karmaPolicy, ConfigMap endpointConfig)
  : listener(*core, port)
{
  this->port           = port;
  this->karmaPolicy    = karmaPolicy;
  this->endpointConfig = std::move(endpointConfig);
  listener.creator     = [this](Core &core) {
    using TXType = std::shared_ptr<google::protobuf::Message>;
    std::cout << "Create endpoint...." << std::endl;
    auto ep0 = std::make_shared<EndpointType<TXType>>(core, 1000000, 1000000, this->endpointConfig);
    auto ep1 = std::make_shared<ProtoMessageEndpoint<TXType>>(std::move(ep0));
    ep1->setup(ep1);
    auto ep2     = std::make_shared<OefAgentEndpoint>(std::move(ep1));
    auto factory = this->factoryCreator(ep2);
    ep2->SetFactory(factory);
    ep2->setup(this->karmaPolicy);
    return ep2;
  };
}

template <template <typename> class EndpointType>
void Oefv1Listener<EndpointType>::start()
{
  listener.start_accept();
}

template class Oefv1Listener<Endpoint>;
// TODO: template class Oefv1Listener<EndpointWebSocket>;
template class Oefv1Listener<EndpointSSL>;
