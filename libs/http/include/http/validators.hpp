#pragma once
#include "variant/variant.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <functional>

namespace fetch
{
namespace http
{
namespace validators
{

struct Validator
{
  byte_array::ConstByteArray description;
  std::function< bool(byte_array::ConstByteArray) > validator;
  variant::Variant schema;
};


inline Validator StringValue(uint16_t min_length = 0, uint16_t max_length = uint16_t(-1))
{
  Validator x;
  x.validator = [](byte_array::ConstByteArray) { return true; };
  x.schema    =  variant::Variant::Object();
  x.schema["type"] = "string";
  x.schema["minLength"] = min_length;
  x.schema["maxLength"] = max_length;  
  return x;
}


}
}
}