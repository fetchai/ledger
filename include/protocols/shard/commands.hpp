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
  HELLO = 2,
  PUSH_TRANSACTION = 3,
  PUSH_BLOCK = 4,
  GET_NEXT_BLOCK = 5,
  
  EXCHANGE_HEADS = 20,
  REQUEST_BLOCKS_FROM = 21,

  LISTEN_TO = 101,
  SET_SHARD_NUMBER = 102,
  SHARD_NUMBER = 103,
  COUNT_OUTGOING_CONNECTIONS = 104  
  
};

};

struct ShardFeed 
{
  
enum 
{
  FEED_BROADCAST_TRANSACTION = 1,
  FEED_BROADCAST_BLOCK = 2,
  FEED_BROADCAST_SHARD_CONNECTION = 3
};

};


}; 
}; 

#endif 
