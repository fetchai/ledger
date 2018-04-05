#ifndef SERVICE_PY_CLIENT_HPP
#define SERVICE_PY_CLIENT_HPP
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"

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
    thread_manager_(4)
  {  }

  ~PyServiceClient()  
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
    function_handler_type const& function, pybind11::args args) 
  {
    std::cout << pybind11::len(args) << std::endl;
    //    byte_array::ByteArray serialised_args;
    //    return client_->CallWithPackedArguments( protocol, function, serialized_args );
    return Promise();
  }
  
  
private:
  fetch::network::ThreadManager thread_manager_;
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


#endif
