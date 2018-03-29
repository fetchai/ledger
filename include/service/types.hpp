#ifndef SERVICE_TYPES_HPP
#define SERVICE_TYPES_HPP
#include "serializer/referenced_byte_array.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/typed_byte_array_buffer.hpp"

namespace fetch {
namespace service {


typedef serializers::TypedByte_ArrayBuffer serializer_type;

  //typedef serializers::ByteArrayBuffer serializer_type;

  
typedef uint64_t protocol_handler_type;
typedef uint64_t function_handler_type;
typedef uint8_t feed_handler_type;  
typedef uint8_t subscription_handler_type;
typedef uint64_t service_classification_type;
};
};
#endif
