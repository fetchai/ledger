#pragma once

namespace fetch {
namespace byte_array {

class ConstByteArray;

} // namespace byte_array
namespace http {

enum class Method
{
  GET     = 1,
  POST    = 2,
  PUT     = 3,
  PATCH   = 4,
  DELETE  = 5,
  OPTIONS = 6
};

char const *ToString(Method method);
bool FromString(byte_array::ConstByteArray const &text, Method &method);

}
}  // namespace fetch
