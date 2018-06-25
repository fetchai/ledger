#ifndef SERVICE_MESSAGE_TYPES_HPP
#define SERVICE_MESSAGE_TYPES_HPP
#include "network/service/types.hpp"

namespace fetch {
namespace service {

service_classification_type const SERVICE_FUNCTION_CALL = 0ull;
service_classification_type const SERVICE_RESULT = 10ull;
service_classification_type const SERVICE_SUBSCRIBE = 20ull;
service_classification_type const SERVICE_UNSUBSCRIBE = 30ull;
service_classification_type const SERVICE_FEED = 40ull;

service_classification_type const SERVICE_ERROR = 999ull;
}
}

#endif
