#ifndef TCP_SERVER_ECHO_HPP
#define TCP_SERVER_ECHO_HPP

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "fetch_asio.hpp" // required to avoid failing build due to -Werror
#include<memory>

namespace fetch {
namespace network {

// Taken from asio example. Echo server modified a little to be self-sufficient (io_context)
class Session
  : public std::enable_shared_from_this<Session>
{
public:
  Session(asio::ip::tcp::tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            //std::cerr << ".";
            do_write(length);
          }
        });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());

    asio::async_write(socket_, asio::buffer(data_, length),
        [this, self, length](std::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            do_read();
          }
        });
  }

  asio::ip::tcp::tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class TcpServerEcho
{
public:
  TcpServerEcho(asio::io_context& io_context, short port)
    : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
  {
    do_accept();
  }

  TcpServerEcho(short port) :
    shared_work_{std::make_shared<asio::io_service::work>(context_)},
    acceptor_(context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
  {
    for (std::size_t i = 0; i < 5; ++i)
    {
      threads_.emplace_back([this](){context_.run();});
    }
    do_accept();
  }

  ~TcpServerEcho()
  {
    shared_work_.reset();
    context_.stop();
    for(auto &i : threads_)
    {
      i.join();
    }
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::tcp::socket socket)
        {
          if (!ec)
          {
            //std::cerr << "-";
            std::make_shared<Session>(std::move(socket))->start();
          } else
          {
            std::cerr << ec.message() << std::endl;
          }

          do_accept();
        });
  }


  asio::io_context                        context_;
  std::shared_ptr<asio::io_service::work> shared_work_;
  asio::ip::tcp::tcp::acceptor            acceptor_;
  std::vector<std::thread>                threads_;
};


}
}

#endif
