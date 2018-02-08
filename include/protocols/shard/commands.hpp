#ifndef PROTOCLS_SHARD_COMMANDS_HPP
#define PROTOCLS_SHARD_COMMANDS_HPP

namespace fetch 
{
namespace protocols 
{
  
enum ShardRPC 
{
  GET_MESSAGES = 1
};
  
enum ShardFeed 
{
  FEED_BROADCAST_TRANSACTION = 1,
  FEED_BROADCAST_BLOCK = 2
};

}; 
}; 

#endif 
