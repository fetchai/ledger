#pragma once

#include <boost/asio.hpp>

class ISocketOwner
{
public:
  using Socket = boost::asio::ip::tcp::socket;

  ISocketOwner() {}
  virtual ~ISocketOwner() {}
  virtual Socket& socket() = 0;
  virtual void go() = 0;
};
