#ifndef RPC_MESSAGE_TYPES_HPP
#define RPC_MESSAGE_TYPES_HPP

namespace fetch {
namespace rpc {
typedef uint64_t rpc_classification_type;

rpc_classification_type const RPC_FUNCTION_CALL = 0ull;
rpc_classification_type const RPC_RESULT = 10ull;
rpc_classification_type const RPC_SUBSCRIBE = 20ull;
rpc_classification_type const RPC_UNSUBSCRIBE = 30ull;
rpc_classification_type const RPC_EVENT = 40ull;
rpc_classification_type const RPC_ERROR = 999ull;
};
};

#endif
