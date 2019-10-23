#include "oef-base/comms/Core.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include <iostream>

static Counter loopstart("mt-core.network.asio.run");
static Counter loopstop("mt-core.network.asio.ended");

Core::Core()
{
  context = std::make_shared<asio::io_context>();
  work    = new asio::io_context::work(*context);
}

Core::~Core()
{
  stop();
}

void Core::run()
{
  loopstart++;
  context->run();
  loopstop++;
}

void Core::stop()
{
  delete work;
  context->stop();
}

std::shared_ptr<tcp::acceptor> Core::makeAcceptor(unsigned short int port)
{
  return std::make_shared<tcp::acceptor>(*context, tcp::endpoint(tcp::v4(), port));
}
