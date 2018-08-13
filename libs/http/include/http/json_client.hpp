#ifndef FETCH_JSON_CLIENT_HPP
#define FETCH_JSON_CLIENT_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "core/script/variant.hpp"
#include "http/method.hpp"
#include "http/client.hpp"

namespace fetch {
namespace http {

class JsonHttpClient
{
public:
  using Variant = script::Variant;
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  explicit JsonHttpClient(std::string host, uint16_t port = 80);
  ~JsonHttpClient() = default;

  bool Get(ConstByteArray const &endpoint, Variant const &request, Variant &response);
  bool Get(ConstByteArray const &endpoint, Variant &response);
  bool Post(ConstByteArray const &endpoint, Variant const &request, Variant &response);
  bool Post(ConstByteArray const &endpoint, Variant &response);

private:

  bool Request(Method method, ConstByteArray const &endpoint, Variant const *request, Variant &response);

  HTTPClient client_;
};

} // namespace http
} // namespace fetch

#endif //FETCH_JSON_CLIENT_HPP
