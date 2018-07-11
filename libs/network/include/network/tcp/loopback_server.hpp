#ifndef TCP_SERVER_LOOPBACK_HPP
#define TCP_SERVER_LOOPBACK_HPP

#include<iostream>
#include<memory>
#include<utility>
#include"network/message.hpp"
#include"network/fetch_asio.hpp" // required to avoid failing build due to -Werror
#include"network/management/network_manager.hpp"

namespace fetch {
namespace network {

std::atomic<std::size_t> openSessions{0};

class BasicLoopback : public std::enable_shared_from_this<BasicLoopback>
{
public:
  BasicLoopback(asio::ip::tcp::tcp::socket socket)
    : socket_(std::move(socket))
  {
    openSessions++;
  }

  ~BasicLoopback()
  {
    openSessions--;
  }

  void Start()
  {
    message_.Resize(lengthPerRead_);
    Read();
  }

private:
  asio::ip::tcp::tcp::socket socket_;
  std::size_t lengthPerRead_ = 1024;
  message_type message_;

  void Read() noexcept
  {
    auto self = shared_from_this();
    socket_.async_read_some(asio::buffer(message_.pointer(), lengthPerRead_),
        [this, self](std::error_code ec, std::size_t length)
        {
          if(!ec)
          {
            Write(length);
          }
        });
  }

  void Write(std::size_t length) noexcept
  {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(message_.pointer(), length),
        //TODO: couple of thigs worth to check here: 1) It does not make sense to pass both - `this` raw pointer and shared_ptr<...> for the same instance, 2) depending on how `asio::async_write(...)` uses instance of passed lambda (who will control lifecycle of lambda instance), it may be necessary to pass here `weak_ptr<>` instead of shared_ptr if lambda nstance is set to `socket_` insatnce (and thus `socket_` would take control over lyfecycle of lambda instace => it would create cyclic reference to `this` instance if we pass here shared_ptr instead of weak)
        [this, self](std::error_code ec, std::size_t)
        {
          if(!ec)
          {
            Read();
          }
        });
  }
};

class LoopbackServer
{
public:
  static constexpr std::size_t DEFAULT_NUM_THREADS = 4;

  explicit LoopbackServer(uint16_t port, std::size_t num_threads = DEFAULT_NUM_THREADS) :
    port_{port},
    networkManager_{num_threads}
  {
    networkManager_.Start();
    networkManager_.Post([this]
    {
      auto strongAccep = networkManager_.CreateIO<asio::ip::tcp::tcp::acceptor>
        (asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_));

      if(strongAccep)
      {
        acceptor_ = strongAccep;
        Accept();
      } else {
        std::cout << "Failed to get acceptor" << std::endl;
        finished_setup_ = true;
      }
    });

    while(!finished_setup_)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }

  ~LoopbackServer()
  {
    acceptor_.reset();
    networkManager_.Stop();
  }

private:
  // IO objects guaranteed to have lifetime less than the io_service/networkManager
  uint16_t                                      port_;
  NetworkManager                                 networkManager_;
  std::weak_ptr<asio::ip::tcp::tcp::acceptor>   acceptor_;
  std::atomic<bool>                             finished_setup_{false};

  void Accept()
  {
    auto strongAccep = acceptor_.lock();
    if(!strongAccep) return;
    strongAccep->async_accept([this, strongAccep](std::error_code ec, asio::ip::tcp::tcp::socket socket)
      {
        if(!ec)
        {
          std::make_shared<BasicLoopback>(std::move(socket))->Start();
        } else {
          fetch::logger.Info("Error in loopback server: ", ec.message());
        }

        Accept();
      });
    finished_setup_ = true;
  }
};


}
}

#endif
