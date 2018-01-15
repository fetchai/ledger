#ifndef RPC_ERROR_CODES_HPP
#define RPC_ERROR_CODES_HPP
#include "serializer/exception.hpp"
namespace fetch {
namespace rpc {
namespace error {
typedef typename serializers::error::error_type error_type;
error_type const ERROR_RPC_PROTOCOL = 1 << 16;  // TODO: move to global place

error_type const USER_ERROR = 0 | ERROR_RPC_PROTOCOL;
error_type const MEMBER_NOT_FOUND = 1 | ERROR_RPC_PROTOCOL;
error_type const MEMBER_EXISTS = 2 | ERROR_RPC_PROTOCOL;

error_type const PROTOCOL_NOT_FOUND = 11 | ERROR_RPC_PROTOCOL;
error_type const PROTOCOL_EXISTS = 12 | ERROR_RPC_PROTOCOL;

error_type const PROMISE_NOT_FOUND = 21 | ERROR_RPC_PROTOCOL;

error_type const UNKNOWN_MESSAGE = 1001 | ERROR_RPC_PROTOCOL;
};
};
};
#endif
