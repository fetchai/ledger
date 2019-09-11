#include "oef-core/comms/Oefv1Listener.hpp"

#include <memory>

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/EndpointWebSocket.hpp"
#include "oef-core/comms/EndpointSSL.hpp"
#include "oef-core/comms/OefAgentEndpoint.hpp"

template <template <typename> class EndpointType>
Oefv1Listener<EndpointType>::Oefv1Listener(std::shared_ptr<Core> core, int port,
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
    ep2->setFactory(factory);
    ep2->setup(this->karmaPolicy);
    return ep2;
  };
}

template <template <typename> class EndpointType>
void Oefv1Listener<EndpointType>::start(void)
{
  listener.start_accept();
}

template class Oefv1Listener<Endpoint>;
template class Oefv1Listener<EndpointWebSocket>;
template class Oefv1Listener<EndpointSSL>;
