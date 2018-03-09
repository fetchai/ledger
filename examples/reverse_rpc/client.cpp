#define FETCH_DISABLE_LOGGING
#include"service_consts.hpp"
#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
#include"commandline/parameter_parser.hpp"
using namespace fetch::commandline;
using namespace fetch::service;
using namespace fetch::byte_array;

class AEA {
public:

  std::string SearchFor(std::string val)
  {
    std::cout << "Searching for " << val << std::endl;
    
    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
    std::string ret = "";

    for(auto &s : strings_ ) 
    {
      if(s.find(val) !=std::string::npos)
      {
        ret = s;
        break;        
      }      
    }
        
    return ret;    
  }

  void AddString(std::string const &s) 
  {
    strings_.push_back( s );    
  }
  
private:
  std::vector< std::string> strings_;
  
  fetch::mutex::Mutex mutex_;
};

class AEAProtocol : public AEA, public Protocol {
public:
  AEAProtocol() : AEA(), Protocol() {
    this->Expose(NodeToAEA::SEARCH, new CallableClassMember<AEA, std::string(std::string)>( this, &AEA::SearchFor) );
  }
};


int main(int argc, char const** argv) 
{
  ParamsParser params;
  params.Parse(argc, argv);

  // Client setup
  fetch::network::ThreadManager tm;  
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, &tm);
  AEAProtocol aea_prot;

  for(std::size_t i = 0; i < params.arg_size(); ++i) {
    aea_prot.AddString( params.GetArg(i) );
  }
  
  tm.Start();

  
  std::this_thread::sleep_for( std::chrono::milliseconds(100) );
  client.Add(FetchProtocols::NODE_TO_AEA, &aea_prot);
  
  auto p =  client.Call( FetchProtocols::AEA_TO_NODE , AEAProtocol::REGISTER );

  if(p.Wait() ) {
    std::cout << "Node registered" << std::endl;
    
    while(true) { }
    
  }
  
  tm.Stop();

  return 0;

}
