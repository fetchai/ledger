#ifndef SERVICE_TCP_SERVICE_CLIENT_HPP
#define SERVICE_TCP_SERVICE_CLIENT_HPP
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"

#include<memory>

namespace fetch
{
namespace service {

class TCPServiceClient
{
public:
  typedef ServiceClient< fetch::network::TCPClient >  client_type;
  typedef std::shared_ptr< client_type > shared_client_type;
  
  TCPServiceClient(TCPServiceClient const &other)  = delete;  
  
  TCPServiceClient() :
    thread_manager_(4)
  {  }

  ~TCPServiceClient()  
  {
    Disconnect();
  }
      
  
  void Connect(std::string const& host, uint16_t port)  
  {
    client_ = std::make_shared< client_type >( host, port, &thread_manager_);    
    thread_manager_.Start();    
  }

  void Disconnect() 
  {
    thread_manager_.Stop();
    client_.reset();
  }

  Promise Call(protocol_handler_type const& protocol,
    function_handler_type const& function, byte_array::ByteArray const&args) 
  {    
    return client_->CallWithPackedArguments( protocol, function, args );    
  }
  
  
private:
  fetch::network::ThreadManager thread_manager_;
  shared_client_type client_;
  
} ;


};
};


#endif
