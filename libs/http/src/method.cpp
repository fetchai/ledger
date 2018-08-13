#include "core/byte_array/const_byte_array.hpp"
#include "http/method.hpp"

namespace fetch {
namespace http {

char const *ToString(Method method)
{
  char const *text = "ERR";
  switch (method)
  {
    case Method::GET:
      text = "GET";
      break;
    case Method::POST:
      text = "POST";
      break;
    case Method::PUT:
      text = "PUT";
      break;
    case Method::PATCH:
      text = "PATCH";
      break;
    case Method::DELETE:
      text = "DELETE";
      break;
    case Method::OPTIONS:
      text = "OPTIONS";
      break;
  }

  return text;
}

bool FromString(byte_array::ConstByteArray const &text, Method &method)
{
  bool success = true;

  if (text == "get")
  {
    method = Method::GET;
  }
  else if (text == "post")
  {
    method = Method::POST;
  }
  else if (text == "put")
  {
    method = Method::PUT;
  }
  else if (text == "patch")
  {
    method = Method::PATCH;
  }
  else if (text == "delete")
  {
    method = Method::DELETE;
  }
  else if (text == "options")
  {
    method = Method::OPTIONS;
  }
  else
  {
    success = false;
  }

  return success;
}

} // namespace http
} // namespace fetch
