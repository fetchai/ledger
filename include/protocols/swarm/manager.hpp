#ifndef SWARM_MANAGER_HPP
#define SWARM_MANAGER_HPP

#include "service/publication_feed.hpp"
#include "protocols/swarm/node_details.hpp"
#include "protocols/swarm/serializers.hpp"

#include<unordered_set>

namespace fetch
{
namespace protocols 
{

class SwarmManager : public fetch::service::HasPublicationFeed 
{
public:
  SwarmManager(NodeDetails const &details)
    : details_(details) { }
  
  uint64_t Ping() 
  {
    std::cout << "PING" << std::endl;
    
    return 1337;
  }

  NodeDetails Hello(uint64_t client, NodeDetails details) 
  {
    client_details_[client] = details;
    return details_;
  }

  std::vector< NodeDetails > SuggestPeers() 
  {
    return peers_with_few_followers_;
  }

  void RequestPeerConnections( NodeDetails details ) 
  {
    
    peers_with_few_followers_.push_back(details);
    if(details.public_key() == details_.public_key()) 
    {
      std::cout << "Discovered myself" << std::endl;
    }
    else 
    {

      if(already_seen_.find( details.public_key() ) == already_seen_.end())
      {
        std::cout << "Discovered " << details.public_key() << std::endl;
        already_seen_.insert( details.public_key() );        
        this->Publish(SwarmFeed::FEED_REQUEST_CONNECTIONS, details);
      }
      else
      {
        std::cout << "Ignored " << details.public_key() << std::endl;
      }
      
       
    }
    

  }
  
  void EnoughPeerConnections( NodeDetails details ) 
  {
    bool found = false;
    auto it = peers_with_few_followers_.end();
    while( it != peers_with_few_followers_.begin() ) 
    {
      --it;
      if( (*it) == details ) 
      {
        found = true;
        peers_with_few_followers_.erase( it );
      }
    }
    
    if(found) 
    {
      this->Publish(SwarmFeed::FEED_ENOUGH_CONNECTIONS, details);
    }
  }

  std::string GetAddress(uint64_t client) 
  {
    if(request_ip_) return request_ip_(client);    

    return "unknown";    
  }

  void SetClientIPCallback( std::function< std::string(uint64_t) > request_ip ) 
  {
    request_ip_ = request_ip;    
  }


  void with_suggestions_do(std::function< void(std::vector< NodeDetails > const &) > fnc) 
  {
    fnc( peers_with_few_followers_ );    
  }

  void with_client_details_do(std::function< void(std::map< uint64_t, NodeDetails > const &) > fnc) 
  {
    fnc( client_details_ );    
  }
  
private:
  NodeDetails const &details_;
  std::vector< NodeDetails > peers_with_few_followers_;
  std::map< uint64_t,  NodeDetails > client_details_;  
  std::function< std::string(uint64_t) > request_ip_;
  std::unordered_set< std::string > already_seen_;
  
};


}; // namespace protocols
}; // namespace fetch
#endif // SWARM_MANAGER_HPP
