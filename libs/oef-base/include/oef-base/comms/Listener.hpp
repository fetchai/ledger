#pragma once
#pragma once

#include "network/fetch_asio.hpp"
#include "oef-base/comms/ISocketOwner.hpp"

#include <memory>

using asio::ip::tcp;

class Core;

class Listener
{
public:
  using CONN_CREATOR = std::function<std::shared_ptr<ISocketOwner>(Core &core)>;

  Listener(Core &core, unsigned short int port);
  virtual ~Listener();

  void start_accept();
  void handle_accept(std::shared_ptr<ISocketOwner> new_connection, std::error_code const &error);

  std::shared_ptr<tcp::acceptor> acceptor;
  CONN_CREATOR                   creator;

private:
  Core &core;
};
