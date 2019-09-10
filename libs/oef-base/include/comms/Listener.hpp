#pragma once
#pragma once

#include <memory>
#include <boost/asio.hpp>

#include "base/src/cpp/comms/ISocketOwner.hpp"

using boost::asio::ip::tcp;

class Core;

class Listener
{
public:
  using CONN_CREATOR = std::function< std::shared_ptr<ISocketOwner> (Core &core)>;


  Listener(Core &core, unsigned short int port);
  virtual ~Listener();

  void start_accept();
  void handle_accept(std::shared_ptr<ISocketOwner> new_connection, const boost::system::error_code& error);

  std::shared_ptr<tcp::acceptor> acceptor;
  CONN_CREATOR creator;
private:
  Core &core;
};
