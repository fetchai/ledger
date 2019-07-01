#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/const_byte_array.hpp"
#include "http/method.hpp"
#include "variant/variant.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace fetch {
namespace http {

class HttpClientInterface;

/**
 * The JsonHttpClient is an adapter around the normal HttpClient. It is used when interacting
 * with Json based APIs. Requests and response objects are converted from and to json before/after
 * the underlying HTTP calls
 */
class JsonClient
{
public:
  using Variant        = variant::Variant;
  using ConstByteArray = byte_array::ConstByteArray;
  using Headers        = std::unordered_map<std::string, std::string>;

  enum class ConnectionMode
  {
    HTTP,
    HTTPS
  };

  // Helpers
  static JsonClient CreateFromUrl(std::string const &url);

  // Construction / Destruction
  JsonClient(ConnectionMode mode, std::string host);
  JsonClient(ConnectionMode mode, std::string host, uint16_t port);
  JsonClient(JsonClient const &) = delete;
  JsonClient(JsonClient &&) noexcept;
  ~JsonClient();

  /// @name Action Methods
  /// @{
  bool Get(ConstByteArray const &endpoint, Variant &response);
  bool Get(ConstByteArray const &endpoint, Headers const &headers, Variant &response);
  bool Post(ConstByteArray const &endpoint, Variant const &request, Variant &response);
  bool Post(ConstByteArray const &endpoint, Variant &response);
  bool Post(ConstByteArray const &endpoint, Headers const &headers, Variant const &request,
            Variant &response);
  bool Post(ConstByteArray const &endpoint, Headers const &headers, Variant &response);
  /// @}

  /// @name Accessors
  /// @{
  HttpClientInterface const &underlying_client() const;
  /// @}

  // Operators
  JsonClient &operator=(JsonClient const &) = delete;
  JsonClient &operator=(JsonClient &&) = default;

private:
  constexpr static char const *LOGGING_NAME = "JsonClient";

  using ClientPtr = std::unique_ptr<HttpClientInterface>;

  bool Request(Method method, ConstByteArray const &endpoint, Headers const *headers,
               Variant const *request, Variant &response);

  ClientPtr client_;
};

}  // namespace http
}  // namespace fetch
