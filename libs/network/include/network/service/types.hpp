#pragma once
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

namespace fetch {
namespace service {

typedef serializers::TypedByte_ArrayBuffer serializer_type;

// typedef serializers::ByteArrayBuffer serializer_type;

typedef uint64_t protocol_handler_type;
typedef uint64_t function_handler_type;
typedef uint8_t  feed_handler_type;
typedef uint8_t  subscription_handler_type;
typedef uint64_t service_classification_type;
}  // namespace service
}  // namespace fetch
