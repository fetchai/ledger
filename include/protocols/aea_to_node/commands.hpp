#ifndef PROTOCLS_AEA_TO_NODE_RPC_COMMANDS_HPP
#define PROTOCLS_AEA_TO_NODE_RPC_COMMANDS_HPP

namespace fetch
{
namespace protocols
{

struct AEAToNodeRPC
{
enum
{
  REGISTER_INSTANCE = 32,
  QUERY, // legacy TODO: (`HUT`) : remove once have full dependence on multi-queries
  QUERY_MULTI,
  REGISTER_FOR_CALLBACKS,
  DEREGISTER_FOR_CALLBACKS,
  BUY,
};

};

};
};

#endif
