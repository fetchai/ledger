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

#include "oef-search/comms/Oefv1Listener.hpp"

#include <memory>

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/Endpoint.hpp"
#include "oef-base/comms/IOefTaskFactory.hpp"
#include "oef-base/proto_comms/ProtoMessageEndpoint.hpp"
#include "oef-base/proto_comms/ProtoPathMessageReader.hpp"
#include "oef-base/proto_comms/ProtoPathMessageSender.hpp"
#include "oef-base/utils/Uri.hpp"
#include "oef-search/comms/OefSearchEndpoint.hpp"

template <template <typename> class EndpointType>
Oefv1Listener<EndpointType>::Oefv1Listener(std::shared_ptr<Core> const &core,
                                           unsigned short int port, ConfigMap endpointConfig)
  : listener(*core, port)
{
  this->port           = port;
  this->endpointConfig = std::move(endpointConfig);
  listener.creator     = [this](Core &core) {
    std::cout << "Create endpoint...." << std::endl;
    using TXType = std::pair<Uri, std::shared_ptr<google::protobuf::Message>>;
    auto ep0 = std::make_shared<EndpointType<TXType>>(core, 1000000, 1000000, this->endpointConfig);
    auto ep1 = std::make_shared<
        ProtoMessageEndpoint<TXType, ProtoPathMessageReader, ProtoPathMessageSender>>(
        std::move(ep0));
    ep1->setup(ep1);

    auto ep2     = std::make_shared<OefSearchEndpoint>(std::move(ep1));
    auto factory = this->factoryCreator(ep2);
    ep2->SetFactory(factory);
    ep2->setup();
    return ep2;
  };
}

template <template <typename> class EndpointType>
void Oefv1Listener<EndpointType>::start()
{
  listener.start_accept();
}

template class Oefv1Listener<Endpoint>;
