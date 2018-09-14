#pragma once
#include"serializers/referenced_byte_array.hpp"
#include"service/service_client.hpp"

#include<memory>

namespace fetch
{
namespace service
{

class PyServiceClient
{
public:
  typedef ServiceClient< fetch::network::TCPClient >  client_type;
  typedef std::shared_ptr< client_type > shared_client_type;
  
  PyServiceClient(PyServiceClient const &other)  = delete;  
  
  PyServiceClient() :
    network_manager_(4)
  {  }

  ~PyServiceClient()  
  {
    Disconnect();
  }
      
  
  void Connect(std::string const& host, uint16_t port)  
  {
    client_ = std::make_shared< client_type >( host, port, &network_manager_);    
    network_manager_.Start();    
  }

  void Disconnect() 
  {
    network_manager_.Stop();
    client_.reset();
  }

  Promise Call(protocol_handler_type const& protocol,
    function_handler_type const& function, pybind11::args args) 
  {
    std::cout << pybind11::len(args) << std::endl;
    //    byte_array::ByteArray serialised_args;
    //    return client_->CallWithPackedArguments( protocol, function, serialized_args );
    return Promise();
  }
  
  
private:
  fetch::network::NetworkManager network_manager_;
  shared_client_type client_;
  
} ;

void BuildClient(pybind11::module &m)
{
  namespace py = pybind11;
  
  py::class_<PyServiceClient>(m, "ServiceClient")
    .def(py::init<>())    
    .def("Connect", &PyServiceClient::Connect)
    .def("Disconnect", &PyServiceClient::Disconnect)
    .def("Call", &PyServiceClient::Call);


}
  
};
};


