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

#include <functional>
#include <iostream>

#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/Listener.hpp"
#include "oef-base/monitoring/Counter.hpp"

static Counter accepting("mt-core.network.accept");
static Counter errored("mt-core.network.accepterror");
static Counter accepted("mt-core.network.accepted");

Listener::Listener(Core &thecore, unsigned short int port)
  : core(thecore)
{
  acceptor = thecore.makeAcceptor(port);
}

void Listener::start_accept()
{
  auto new_connection = creator(core);
  accepting++;
  acceptor->async_accept(
      new_connection->socket(),
      std::bind(&Listener::handle_accept, this, new_connection, std::placeholders::_1));
}

void Listener::handle_accept(std::shared_ptr<ISocketOwner> const &new_connection,
                             std::error_code const &              error)
{
  accepted++;
  if (!error)
  {
    new_connection->go();
    start_accept();
  }
  else
  {
    errored++;
    std::cout << "Listener::handle_accept " << error << std::endl;
  }
}
