#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP
#include"http/http_connection_manager.hpp"
#include "http/abstract_connection.hpp"
#include "http/request.hpp"
#include "mutex.hpp"
#include "assert.hpp"
#include "logger.hpp"

#include <asio.hpp>
#include<memory>
namespace fetch {
namespace http {

class HTTPConnection : public AbstractHTTPConnection,
                       public std::enable_shared_from_this<HTTPConnection> 
{
public:
  typedef typename AbstractHTTPConnection::shared_type connection_type;  
  typedef HTTPConnectionManager::handle_type handle_type;
  typedef std::shared_ptr< HTTPRequest > shared_request_type;
  typedef std::shared_ptr< asio::streambuf > buffer_ptr_type;
  
  
  HTTPConnection(asio::ip::tcp::tcp::socket socket, HTTPConnectionManager& manager)
    : socket_(std::move(socket)),
      manager_(manager),
      write_mutex_(__LINE__, __FILE__)
  {
    fetch::logger.Debug("HTTP connection from ", socket_.remote_endpoint().address().to_string() );
  }

  ~HTTPConnection() 
  {
    manager_.Leave(handle_);
  }
  
  void Start() 
  {
    handle_ = manager_.Join(shared_from_this());
    ReadHeader();
  }

  void Send(AbstractHTTPResponse const&) override 
  {
    TODO("Not implemented");    
  }
  
  
  std::string Address() override
  {
    return socket_.remote_endpoint().address().to_string();    
  }

  asio::ip::tcp::tcp::socket& socket() { return socket_; }
  
public:
  void ReadHeader(buffer_ptr_type buffer_ptr = nullptr) 
  {
    shared_request_type request = std::make_shared< HTTPRequest >();
    if(!buffer_ptr ) buffer_ptr = std::make_shared< asio::streambuf  >(  std::numeric_limits<std::size_t>::max() );
    
    auto cb = [this, buffer_ptr, request](std::error_code const &ec, std::size_t const &len) {

      if(ec) {
        this->HandleError(ec, request);
        return;          
      }
      else
      {
        
        HTTPRequest::SetHeader( *request, *buffer_ptr,  len );        
        ReadBody( buffer_ptr, request );          
      }
    };

    asio::async_read_until(socket_, *buffer_ptr, "\r\n\r\n", cb);        
  }

  void ReadBody(buffer_ptr_type buffer_ptr, shared_request_type request) 
  {
    
    if( request->content_length() <= buffer_ptr->size() ) {
      HTTPRequest::SetBody( *request, *buffer_ptr);
      
      ReadHeader(buffer_ptr); 
      return;      
    }
        
    auto cb = [this, buffer_ptr, request](std::error_code const &ec, std::size_t const &len) {
      if(ec) {
        this->HandleError(ec, request);
        return;
      }
      
    };
    
    std::cout << "reading remaining" << std::endl;
    std::cout << "TODO: check the internals of async read" << std::endl;    
    asio::async_read(socket_, *buffer_ptr, asio::transfer_exactly( request->content_length() - buffer_ptr->size()), cb);    
  }
  

  void HandleError(std::error_code const &ec, shared_request_type req) 
  {
    std::cout << "TODO: Handle error" << std::endl;
    Close();    
  }
  
  
  void Write() 
  {
    // TODO
  }

  void Close() {
    manager_.Leave( handle_ );
  }
  
  asio::ip::tcp::tcp::socket socket_;
  HTTPConnectionManager &manager_;
//  chunk_queue_type write_queue_;  
  fetch::mutex::Mutex write_mutex_;

  handle_type handle_;  
};

  
};
};


#endif
