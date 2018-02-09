#ifndef PROTOCLS_SWARM_COMMANDS_HPP
#define PROTOCLS_SWARM_COMMANDS_HPP

namespace fetch 
{
namespace protocols 
{

struct SwarmRPC 
{
 
enum 
{
  PING = 1,
  HELLO = 2,  
  SUGGEST_PEERS = 3,
  REQUEST_PEER_CONNECTIONS = 4,
  DISCONNECT_FEED = 6,
  WHATS_MY_IP = 7
};

};

struct SwarmFeed 
{
 
enum 
{
  FEED_REQUEST_CONNECTIONS = 1,
  FEED_ENOUGH_CONNECTIONS = 2,  
  FEED_ANNOUNCE_NEW_COMER = 3
};

};

}; 
}; 

#endif 
