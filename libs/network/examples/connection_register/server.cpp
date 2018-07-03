#include<iostream>
#include"network/service/server.hpp"
#include"service_consts.hpp"
#include"node_details.hpp"
using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;

template< typename D >
class AuthenticationLogic {
public:
  typedef D node_details_type;
  
  uint64_t Ping() 
  {
    return 1337;
  }

  byte_array::ByteArray Hello(network::AbstractConnection::connection_handle_type const &client) 
  {
    return byte_array::ByteArray(); 
  }

  byte_array::ByteArray GetChallenge(network::AbstractConnection::connection_handle_type const &client) 
  {
    return byte_array::ByteArray(); 
  }

  void RespondToChallenge(network::AbstractConnection::connection_handle_type const &client, byte_array::ByteArray const &response) 
  {

  } 
};

template< typename D >
class AuthenticationProtocol :  public Protocol {
public:
  
  AuthenticationProtocol() : Protocol() {
    this->Expose(PING, auth_logic_, &AuthenticationLogic<D>::Ping);    
    this->ExposeWithClientArg(HELLO, auth_logic_, &AuthenticationLogic<D>::Hello );
    this->ExposeWithClientArg(GET_CHALLENGE, auth_logic_, &AuthenticationLogic<D>::GetChallenge );
    this->ExposeWithClientArg(RESPOND_TO_CHALLENGE, auth_logic_, &AuthenticationLogic<D>::RespondToChallenge );

  }
private:
  AuthenticationLogic<D> *auth_logic_;  
};


class TestLogic 
{
public:
  std::string Greet(std::string const &name) 
  {
    return "Hello, " + name;
  }
    
  int Add(int a, int b) 
  {
    return a + b;
  }
  
};  

class TestProtocol :  public Protocol {
public:
  TestProtocol() : Protocol() {
    this->Expose(GREET, &test_, &TestLogic::Greet);    
    this->Expose(ADD, &test_, &TestLogic::Add);    
  }
private:
  TestLogic test_;
};

class ProtectedService : public ServiceServer< fetch::network::TCPServer > {
public:
  ProtectedService(uint16_t port, fetch::network::ThreadManager tm) : ServiceServer(port, tm) {
    test_proto_.AddMiddleware([](network::AbstractConnection::connection_handle_type const &n, byte_array::ByteArray const &data) {
        std::cout << "Invoking middleware for the TEST protocol." << std::endl;
        throw serializers::SerializableException(
          0, byte_array_type("You don't have access "));
      });
    
    this->Add(AUTH, &auth_proto_ );
    this->Add(TEST, &test_proto_);  
  }
private:
  AuthenticationProtocol<fetch::NodeDetails> auth_proto_;  
  TestProtocol test_proto_;
};


int main() {
  fetch::network::ThreadManager tm(8);  
  ProtectedService serv(8080, tm);
  fetch::network::ConnectionRegister<fetch::NodeDetails> creg;
  serv.SetConnectionRegister(creg);
  
  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  tm.Stop();

  return 0;

}
