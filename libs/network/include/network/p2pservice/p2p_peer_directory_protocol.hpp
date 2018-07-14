#ifndef NETWORK_P2PSERVICE_P2P_PEER_DIRECTORY_PROTOCOL_HPP
#define NETWORK_P2PSERVICE_P2P_PEER_DIRECTORY_PROTOCOL_HPP

namespace fetch
{
namespace ledger
{
class P2PPeerDirectory;

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
    Protocol::Expose(NEED_CONNECTIONS, directory_,
                     &P2PPeerDirectory::NeedConnections);
    Protocol::Expose(ENOUGH_CONNECTIONS, directory_,
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
