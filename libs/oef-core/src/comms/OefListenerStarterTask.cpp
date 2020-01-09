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

#include "oef-base/comms/Endpoint.hpp"
#include "oef-base/comms/EndpointWebSocket.hpp"
#include "oef-base/comms/OefListenerSet.hpp"
#include "oef-base/threading/ExitState.hpp"
#include "oef-core/comms/EndpointSSL.hpp"
#include "oef-core/comms/OefListenerStarterTask.hpp"
#include "oef-core/comms/Oefv1Listener.hpp"

template <template <typename> class EndpointType>
fetch::oef::base::ExitState OefListenerStarterTask<EndpointType>::run()
{
  // open port here.
  auto result = std::make_shared<Oefv1Listener<EndpointType>>(core, p, karmaPolicy, endpointConfig);

  result->factoryCreator = initialFactoryCreator;

  result->start();

  // when done add to the listeners
  listeners->add(p, result);
  return fetch::oef::base::ExitState::COMPLETE;
}

template class OefListenerStarterTask<Endpoint>;
// TODO: template class OefListenerStarterTask<EndpointWebSocket>;
template class OefListenerStarterTask<EndpointSSL>;
