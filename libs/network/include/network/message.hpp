#pragma once
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <deque>

namespace fetch {
namespace network {
using message_type = byte_array::ByteArray;
using message_queue_type = std::deque<message_type>;
}  // namespace network
}  // namespace fetch
