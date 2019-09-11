#include "oef-core/comms/OefListenerStarterTask.hpp"

#include <iostream>

#include "mt-core/secure/experimental/cpp/EndpointSSL.hpp"
#include "oef-base/comms/Endpoint.hpp"
#include "oef-base/comms/EndpointWebSocket.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-core/comms/Oefv1Listener.hpp"

template <template <typename> class EndpointType>
ExitState OefListenerStarterTask<EndpointType>::run(void)
{
  // open port here.
  auto result = std::make_shared<Oefv1Listener<EndpointType>>(core, p, karmaPolicy, endpointConfig);

  result->factoryCreator = initialFactoryCreator;

  result->start();

  // when done add to the listeners
  listeners->add(p, result);
  return ExitState::COMPLETE;
}

template class OefListenerStarterTask<Endpoint>;
template class OefListenerStarterTask<EndpointWebSocket>;
template class OefListenerStarterTask<EndpointSSL>;
