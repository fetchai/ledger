#pragma once

#include "network/fetch_asio.hpp"
#include <atomic>
#include <memory>
#include <mutex>

using asio::ip::tcp;

class Core
{
public:
  Core();
  virtual ~Core();

  void run(void);
  void stop(void);

  operator asio::io_context *()
  {
    return context.get();
  }
  operator asio::io_context &()
  {
    return *context;
  }

  std::shared_ptr<tcp::acceptor> makeAcceptor(unsigned short int port);

private:
  std::shared_ptr<asio::io_context> context;
  asio::io_context::work *          work;
};
