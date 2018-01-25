#ifndef SERVICE_MESSAGE_HPP
#define SERVICE_MESSAGE_HPP

#include <deque>
#include "byte_array/referenced_byte_array.hpp"
namespace fetch {
namespace service {
typedef byte_array::ReferencedByteArray message_type;
typedef std::deque<message_type> message_queue_type;
};
};

#endif
