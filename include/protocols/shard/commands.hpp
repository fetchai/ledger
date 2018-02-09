#ifndef PROTOCLS_SHARD_COMMANDS_HPP
#define PROTOCLS_SHARD_COMMANDS_HPP

namespace fetch 
{
namespace protocols 
{

struct ShardRPC 
{
  
enum 
{
  PING = 1,
  PUSH_TRANSACTION = 2,
  PUSH_BLOCK = 3,
  GET_NEXT_BLOCK = 4,
  COMMIT = 5,
  LISTEN_TO = 6
};

};

struct ShardFeed 
{
  
enum 
{
  FEED_BROADCAST_TRANSACTION = 1,
  FEED_BROADCAST_BLOCK = 2
};

};


}; 
}; 

#endif 
