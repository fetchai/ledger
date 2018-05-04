#ifndef SERVICE_TYPES_HPP
#define SERVICE_TYPES_HPP
#include "serializers/byte_array_buffer.hpp"
#include "serializers/referenced_byte_array.hpp"
#include "serializers/stl_types.hpp"
#include "serializers/typed_byte_array_buffer.hpp"

namespace fetch {
namespace service {

typedef serializers::TypedByte_ArrayBuffer serializer_type;

// typedef serializers::ByteArrayBuffer serializer_type;

typedef uint64_t protocol_handler_type;
typedef uint64_t function_handler_type;
typedef uint8_t feed_handler_type;
typedef uint8_t subscription_handler_type;
typedef uint64_t service_classification_type;
}
}
#endif
