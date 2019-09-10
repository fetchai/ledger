#include <functional>
#include <iostream>

#include "Core.hpp"
#include "Listener.hpp"
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

void Listener::handle_accept(std::shared_ptr<ISocketOwner>    new_connection,
                             const boost::system::error_code &error)
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

Listener::~Listener()
{}
