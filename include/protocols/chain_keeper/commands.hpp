#ifndef PROTOCLS_CHAIN_KEEPER_COMMANDS_HPP
#define PROTOCLS_CHAIN_KEEPER_COMMANDS_HPP

namespace fetch 
{
namespace protocols 
{

struct ChainKeeperRPC 
{
  
enum 
{
  PING = 1,
  HELLO = 2,
  PUSH_TRANSACTION = 3,
  GET_TRANSACTIONS = 4,
  
  PUSH_BLOCK = 5,
  GET_NEXT_BLOCK = 6,
  GET_BLOCKS = 7, 
  
  EXCHANGE_HEADS = 20,
  REQUEST_BLOCKS_FROM = 21,

  LISTEN_TO = 101,
  SET_GROUP_NUMBER = 102,
  GROUP_NUMBER = 103,
  COUNT_OUTGOING_CONNECTIONS = 104  
  
};

};

struct ChainKeeperFeed 
{
  
enum 
{
  FEED_NEW_BLOCKS = 1,
};

};


}; 
}; 

#endif 
