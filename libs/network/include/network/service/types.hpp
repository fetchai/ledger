#pragma once
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

namespace fetch {
namespace service {

using serializer_type = serializers::TypedByte_ArrayBuffer;

// using serializer_type = serializers::ByteArrayBuffer;

using protocol_handler_type       = uint64_t;
using function_handler_type       = uint64_t;
using feed_handler_type           = uint8_t;
using subscription_handler_type   = uint8_t;
using service_classification_type = uint64_t;
}  // namespace service
}  // namespace fetch
