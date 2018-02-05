#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP
#include"http/http_connection_manager.hpp"
#include"http/connection.hpp"
#include"network/thread_manager.hpp"
#include"http/route.hpp"

#include<deque>
#include<functional>
#include<vector>
#include<map>
#include<regex>

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

  typedef std::function< void(HTTPRequest& ) > request_middleware_type;
  typedef std::function< HTTPResponse(ViewParameters, HTTPRequest) > view_type;
  typedef std::function< void(HTTPResponse&, HTTPRequest const& ) > response_middleware_type;

  struct MountedView 
  {
    Route route;
    view_type view;    
  };
    
  
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

  void PushRequest(handle_type client, HTTPRequest req) override
  {
    // TODO: improve such that it works for multiple threads.
    eval_mutex_.lock();    
    for(auto &m : pre_view_middleware_)
    {
      m( req );      
    }

    HTTPResponse res( "page not found", fetch::http::mime_types::GetMimeTypeFromExtension(".html"), http::status_code::CLIENT_ERROR_NOT_FOUND ) ;   
    ViewParameters params;
    
    for(auto &v: views_)
    {
      if(v.route.Match( req.uri(), params ))
      {
        res = v.view( params, req);
        break;        
      }
    }
        
    for(auto &m : post_view_middleware_)
    {
      m( res, req );      
    }
    
    eval_mutex_.unlock();    

    manager_->Send( client, res );
    
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

  void AddMiddleware( request_middleware_type const &middleware )
  {
    pre_view_middleware_.push_back( middleware );    
  }

  void AddMiddleware( response_middleware_type const &middleware )
  {
    post_view_middleware_.push_back( middleware );
  }

  void AddView( byte_array::ByteArray const& path,  view_type const &view )
  {
    
    views_.push_back( { Route::FromString(path), view } );    
  }
  
private:
  fetch::mutex::Mutex eval_mutex_;
  
  std::vector< request_middleware_type > pre_view_middleware_;
  std::vector< MountedView > views_;  
  std::vector< response_middleware_type > post_view_middleware_;  
  
  
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
