#ifndef LEDGER_CHAIN_MAIN_CHAIN_SERVICE_HPP
#define LEDGER_CHAIN_MAIN_CHAIN_SERVICE_HPP

#include"storage/object_store.hpp"
#include"storage/object_store_protocol.hpp"
#include"network/service/server.hpp"
#include"network/details/thread_pool.hpp"

#include"network/management/connection_register.hpp"
#include"ledger/chain/main_chain.hpp"
#include"ledger/chain/main_chain_protocol.hpp"
#include"ledger/chain/main_chain_details.hpp"
#include"ledger/chain/main_chain_identity.hpp"
#include"ledger/chain/main_chain_identity_protocol.hpp"
#include"ledger/chain/main_chain_controller.hpp"
#include"ledger/chain/main_chain_controller_protocol.hpp"


namespace fetch
{
namespace chain
{

class MainChainService : public service::ServiceServer< fetch::network::TCPServer >
{
public:

  using mainchain_type = fetch::chain::MainChain;
  using mainchain_protocol_type = fetch::chain::MainChainProtocol;

  using connectivity_details_type = MainChainDetails;
  using client_register_type = fetch::network::ConnectionRegister< connectivity_details_type >;

  using controller_type = MainChainController;
  using controller_protocol_type = MainChainControllerProtocol;

  using identity_type = MainChainIdentity;
  using identity_protocol_type = MainChainIdentityProtocol;
  using super_type = service::ServiceServer< fetch::network::TCPServer >;

  using thread_pool_type = network::ThreadPool;  

  enum {
    IDENTITY = 1,
    CHAIN,
    CONTROLLER
  };
  
  MainChainService(std::string const &db_dir, 
    uint16_t port, fetch::network::NetworkManager tm, bool start_sync = true)
    : super_type(port, tm) {

    fetch::logger.Warn("Establishing mainchain Service on rpc://127.0.0.1:", port);

    thread_pool_ = network::MakeThreadPool(1);

    // format and generate the prefix
    std::string prefix;
    {
      std::stringstream s;
      s << db_dir;
      s << "_mainchain" << std::setw(3) << std::setfill('0') << "_";
      prefix = s.str();
    }

    // Main chain Identity
    identity_ = std::make_shared<identity_type>(register_, tm);    
    identity_protocol_.reset(new identity_protocol_type(identity_.get()));
    this->Add(IDENTITY, identity_protocol_.get());

    mainchain_.reset( new mainchain_type() );
    mainchain_protocol_.reset( new mainchain_protocol_type( mainchain_.get() ) );
    this->Add(CHAIN, mainchain_protocol_.get());

    controller_.reset(new controller_type(IDENTITY, identity_, register_, tm));
    controller_protocol_.reset(new controller_protocol_type(controller_.get()));
    this->Add(CONTROLLER, controller_protocol_.get());

    thread_pool_->Start();    
  }

  ~MainChainService() 
  {

    identity_protocol_.reset();
    identity_.reset();

  }


private:
  client_register_type register_;
  thread_pool_type thread_pool_;  

  std::shared_ptr<identity_type> identity_;
  std::unique_ptr<identity_protocol_type> identity_protocol_;

  std::unique_ptr< mainchain_type > mainchain_;
  std::unique_ptr< mainchain_protocol_type > mainchain_protocol_;

  std::unique_ptr<controller_type> controller_;
  std::unique_ptr<controller_protocol_type> controller_protocol_;
  

};

}
}

#endif
