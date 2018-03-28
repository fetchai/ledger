#include"swarm_service.hpp"
#include"shard_service.hpp"
#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols.hpp"
#include"commandline/parameter_parser.hpp"

#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"
#include<vector>
#include<memory> 

using namespace fetch::commandline;
using namespace fetch::protocols;

class FetchLedger 
{
public:
  FetchLedger(uint16_t offset, std::string const &name, std::size_t const &shards ) :
    thread_manager_( new fetch::network::ThreadManager(64) ),
    controller_( 1337 + offset, 7070 + offset, name, thread_manager_ )
  {
    for(std::size_t i=0; i < shards; ++i)
    {
      std::size_t j =  offset * shards + i;      
      shards_.push_back( std::make_shared< FetchChainKeeperService > (4000 + j, 9590 + j, thread_manager_ ));
    }

    start_event_ = thread_manager_->OnAfterStart([this, shards, offset]() {
        
        thread_manager_->io_service().post([this]() {
            this->ConnectChainKeepers();
          });
      });
    
    stop_event_ = thread_manager_->OnBeforeStop([this]() {
//        for(auto &s: shards_) {
          // TODO disconnect
//        }
        
        shards_.clear();        
      });
  }
  
  ~FetchLedger() 
  {
    thread_manager_->Off( start_event_ );
    thread_manager_->Off( stop_event_ );
  }
  
  
  void Start() 
  {
    thread_manager_->Start();
  }

  void Stop() 
  {
    thread_manager_->Stop();
  }
  
  void Bootstrap(std::string const &address, uint16_t const &port) 
  {
    controller_.Bootstrap( address, port );
  }
  
private:

  void ConnectChainKeepers() 
  {
    std::cout << "Connecting shards" << std::endl;
    uint32_t i = 0;
    controller_.SetGroupParameter( uint32_t(shards_.size()) );
    
    for(auto &s: shards_) {
      std::cout << " - localhost " <<  s->port() << std::endl;
      auto client = controller_.ConnectChainKeeper( "localhost", s->port() );

      client->Call(fetch::protocols::FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::SET_GROUP_NUMBER, i, uint32_t(shards_.size()) );
      ++i;
    }
  } 
  
  fetch::network::ThreadManager *thread_manager_;      
  FetchSwarmService controller_;
  std::vector< std::shared_ptr< FetchChainKeeperService > > shards_;

  typename fetch::network::ThreadManager::event_handle_type start_event_;
  typename fetch::network::ThreadManager::event_handle_type stop_event_;      
};



int main(int argc, char const** argv) 
{
  ParamsParser params;
  params.Parse(argc, argv);
 
  if(params.arg_size() < 4) 
  {
    std::cout << "usage: " << argv[0] << " [port offset] [info] [shards] [[bootstrap_host] [bootstrap_port]]" << std::endl;
    exit(-1);
  }
  
  uint16_t  my_port = params.GetArg<uint16_t>(1);
  std::string  info = params.GetArg(2);
  uint16_t  shards = params.GetArg<uint16_t>(3);  

  FetchLedger service(my_port, info, shards);
  service.Start();
  
  std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );  
  
  if(params.arg_size() >= 5) 
  {
    std::string host = params.GetArg(4);
    uint16_t port = params.GetArg<uint16_t>(5);
    std::cout << "Bootstrapping through " << host << " " << port << std::endl;
    service.Bootstrap(host, port);
  }
  
  
  std::cout << "Ctrl-C to stop" << std::endl;
  while(true) 
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
  }
  return 0;
}
