#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP
#include"http/http_connection_manager.hpp"
#include"http/connection.hpp"
#include"network/thread_manager.hpp"

#include<deque>

namespace fetch
{
namespace http
{

class HTTPServer : public AbstractHTTPServer 
{
public:
  typedef uint64_t handle_type; // TODO: Make global definition

  typedef network::ThreadManager thread_manager_type;  
  typedef thread_manager_type* thread_manager_ptr_type;
  typedef typename thread_manager_type::event_handle_type event_handle_type;

  
  HTTPServer(uint16_t const &port, thread_manager_ptr_type const &thread_manager) :
    thread_manager_(thread_manager),
    request_mutex_(__LINE__, __FILE__),
    acceptor_(thread_manager->io_service(), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
    socket_(thread_manager->io_service())    
  {
    event_service_start_ = thread_manager->OnBeforeStart([this]() { this->Accept(); } );
    // TODO: If manager running -> Accept();
    manager_ = new HTTPConnectionManager(*this);        
  }

  ~HTTPServer() 
  {
    thread_manager_->Off(event_service_start_);
    if(manager_ != nullptr) delete manager_;    
    socket_.close();
  }

  void PushRequest(handle_type client, HTTPRequest const& req) override
  {
    std::cout << "Received request" << std::endl;   
  }
  
  void Accept()
  {
    auto cb = [this](std::error_code ec) {

      if (!ec) {
        std::make_shared<HTTPConnection>(std::move(socket_), *manager_)
            ->Start();
      }

      Accept();
    };

    acceptor_.async_accept(socket_, cb);   
  }
  
  thread_manager_ptr_type thread_manager_;
  event_handle_type event_service_start_;    
  std::deque<HTTPRequest> requests_;
  fetch::mutex::Mutex request_mutex_;  
  asio::ip::tcp::tcp::acceptor acceptor_;
  asio::ip::tcp::tcp::socket socket_;  
  HTTPConnectionManager* manager_;
};

};
};


#endif
