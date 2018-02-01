#ifndef PROTOCOLS_HPP
#define PROTOCOLS_HPP

namespace fetch 
{
namespace protocols 
{
  
enum DiscoveryRPC 
{
  PING = 1,
  HELLO = 2,  
  SUGGEST_PEERS = 3,
  REQUEST_PEER_CONNECTIONS = 4,
  DISCONNECT_FEED = 6,
  WHATS_MY_IP = 7
};
  
enum DiscoveryFeed 
{
  FEED_REQUEST_CONNECTIONS = 1,
  FEED_ENOUGH_CONNECTIONS = 2,  
  FEED_ANNOUNCE_NEW_COMER = 3
};

}; // namespace fetch::protocols
}; // namespace fetch

#endif // PROTOCOLS_HPP
