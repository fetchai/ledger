#ifndef PROTOCLS_NODE_TO_NODE_RPC_COMMANDS_HPP
#define PROTOCLS_NODE_TO_NODE_RPC_COMMANDS_HPP

namespace fetch
{
namespace protocols
{

struct NodeToNodeRPC
{
enum
{
  GET_INSTANCE = 7,
  PING,
  QUERY,
  FORWARD_QUERY,
  RETURN_QUERY,
  DBG_ADD_ENDPOINT,
  DBG_GET_HISTORY, // not implemented
  DBG_ADD_HISTORY, // not implemented
  DBG_GET_AGENTS, // not implemented
  DBG_ADD_AGENT,
  DBG_LOG_EVENT
};

};

};
};

#endif
