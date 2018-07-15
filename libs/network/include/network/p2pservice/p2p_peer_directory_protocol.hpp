#ifndef NETWORK_P2PSERVICE_P2P_PEER_DIRECTORY_PROTOCOL_HPP
#define NETWORK_P2PSERVICE_P2P_PEER_DIRECTORY_PROTOCOL_HPP
#include"network/p2pservice/p2p_peer_directory.hpp"

namespace fetch
{
namespace p2p
{

class P2PPeerDirectoryProtocol : public fetch::service::Protocol
{
public:
  enum {
    SUGGEST_PEERS = 1,
    NEED_CONNECTIONS,
    ENOUGH_CONNECTIONS
  };
      
  P2PPeerDirectoryProtocol(P2PPeerDirectory *directory)
    : directory_(directory) 
  {

    // RPC
    Protocol::ExposeWithClientArg(NEED_CONNECTIONS, directory_,
                     &P2PPeerDirectory::NeedConnections);
    Protocol::ExposeWithClientArg(ENOUGH_CONNECTIONS, directory_,
                     &P2PPeerDirectory::EnoughConnections); 
    Protocol::Expose(SUGGEST_PEERS, directory_,
                     &P2PPeerDirectory::SuggestPeersToConnectTo);
    
    // Feeds
    Protocol::RegisterFeed(P2PPeerDirectory::FEED_REQUEST_CONNECTIONS, directory_);
    Protocol::RegisterFeed(P2PPeerDirectory::FEED_ENOUGH_CONNECTIONS, directory_);
  }

private:
  P2PPeerDirectory *directory_;
  
};


}
}

#endif
