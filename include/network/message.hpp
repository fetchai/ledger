#ifndef NETWORK_MESSAGE_HPP
#define NETWORK_MESSAGE_HPP

#include <deque>
#include "byte_array/const_byte_array.hpp"
namespace fetch {
namespace network {
typedef byte_array::ByteArray message_type;
typedef std::deque<message_type> message_queue_type;
};
};

#endif
