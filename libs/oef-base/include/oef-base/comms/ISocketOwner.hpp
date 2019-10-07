#pragma once

#include "network/fetch_asio.hpp"

class ISocketOwner
{
public:
  using Socket = asio::ip::tcp::socket;

  ISocketOwner()
  {}
  virtual ~ISocketOwner()
  {}
  virtual Socket &socket() = 0;
  virtual void    go()     = 0;
};
