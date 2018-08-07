#pragma once
#include <utility>

#include "ledger/chain/main_chain_details.hpp"
#include "ledger/chain/main_chain_identity.hpp"
#include "ledger/chain/main_chain_identity_protocol.hpp"
#include "network/management/connection_register.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
#include "network/service/client.hpp"

namespace fetch {
namespace chain {

class MainChainController
{
public:
  using connectivity_details_type  = MainChainDetails;
  using client_type                = fetch::network::TCPClient;
  using service_client_type        = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using client_register_type       = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type       = fetch::network::NetworkManager;
  using mutex_type                 = fetch::mutex::Mutex;
  using connection_handle_type     = client_register_type::connection_handle_type;
  using protocol_handler_type      = service::protocol_handler_type;

  MainChainController(protocol_handler_type const &    identity_protocol,
                      std::weak_ptr<MainChainIdentity> identity, client_register_type reg,
                      network_manager_type const &nm,
                      generics::SharedWithLock<MainChainDetails> my_details
                      )
    : identity_protocol_(identity_protocol)
    , register_(std::move(reg))
    , manager_(nm)
    , my_details_(my_details)
  {
    
  }

  /// External controls
  /// @{
  void RPCConnect(byte_array::ByteArray const &host, uint16_t const &port) { Connect(host, port); }

  void TryConnect(p2p::EntryPoint const &ep)
  {

    for (auto &h : ep.host)
    {
      fetch::logger.Debug("Mainchain trying to connect to ", h, ":", ep.port);
      if (Connect(h, ep.port)) break;
    }
  }

  void Shutdown() { TODO_FAIL("Needs to be implemented"); }

  void StartSync() { TODO_FAIL("Needs to be implemented"); }

  void StopSync() { TODO_FAIL("Needs to be implemented"); }

  int IncomingPeers()
  {
    std::lock_guard<mutex_type> lock_(services_mutex_);
    int                         incoming = 0;
    for (auto &peer : services_)
    {
      auto details = register_.GetDetails(peer.first);
      {
        if (details->is_peer && (!details->is_outgoing))
        {
          ++incoming;
        }
      }
    }
    return incoming;
  }

  int OutgoingPeers()
  {
    std::lock_guard<mutex_type> lock_(services_mutex_);
    int                         outgoing = 0;
    for (auto &peer : services_)
    {
      auto details = register_.GetDetails(peer.first);
      {
        if (details->is_peer && details->is_outgoing)
        {
          ++outgoing;
        }
      }
    }
    return outgoing;
  }

  /// @}

  /// Internal controls
  /// @{
  shared_service_client_type GetClient(connection_handle_type const &n)
  {
    std::lock_guard<mutex_type> lock_(services_mutex_);
    return services_[n];
  }

  shared_service_client_type Connect(byte_array::ByteArray const &host, uint16_t const &port)
  {
    shared_service_client_type client =
        register_.CreateServiceClient<client_type>(manager_, host, port);

    logger.Warn("CONNECT ", std::string(host), ":", port);

    // Waiting for connection to be open
    std::size_t n = 0;
    while (n < 10)
    {
      auto p = client->Call(identity_protocol_, MainChainIdentityProtocol::PING);
      if (p.Wait(100, false))
      {
        if (p.As<MainChainIdentity::ping_type>() != MainChainIdentity::PING_MAGIC)
        {
          n = 10;
        }
        break;
      }
      ++n;
    }

    if (n >= 10)
    {
      logger.Warn("Connection timed out - closing");
      client->Close();
      client.reset();
      return nullptr;
    }

    // Exchaning info
    {
      std::lock_guard<mutex_type> lock_(services_mutex_);
      services_[client->handle()] = client;
    }

    MainChainDetails copy_of_my_details;
    my_details_.CopyOut(copy_of_my_details);

    auto remote_details_promise = client->Call(identity_protocol_, MainChainIdentityProtocol::EXCHANGE_DETAILS, copy_of_my_details);
    auto status = remote_details_promise.WaitLoop(100, 10);

    switch(status)
    {
    case service::Promise::OK:
      break;
    default:
      logger.Warn("While exchanging IDENTITY DETAILS:", service::Promise::DescribeStatus(status));
      client->Close();
      client.reset();
      return nullptr;
    }

    auto details_supplied_by_remote = remote_details_promise.As<MainChainDetails>();

    auto local_name  = std::string(byte_array::ToBase64(my_details_.Lock()->owning_discovery_service_identity.identifier()));
    auto remote_name = std::string(byte_array::ToBase64(details_supplied_by_remote.owning_discovery_service_identity.identifier()));

    logger.Warn("OMG LOCAL  NAME IS:", local_name);
    logger.Warn("OMG REMOTE NAME IS:", remote_name);

    // Setting up details such that the rest of the mainchain what kind of
    // connection we are dealing with.
    auto remote_details = register_.GetDetails(client -> handle());
    //remote_details.
    remote_details -> is_outgoing = true;
    remote_details -> is_peer     = true;

    return client;
  }

  /// @}
private:

  protocol_handler_type            identity_protocol_;
  client_register_type             register_;
  network_manager_type             manager_;
  generics::SharedWithLock<MainChainDetails> my_details_;

  mutex::Mutex                                                           services_mutex_{ __LINE__, __FILE__ };
  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
  std::vector<connection_handle_type>                                    inactive_services_;
};

}  // namespace chain
}  // namespace fetch
