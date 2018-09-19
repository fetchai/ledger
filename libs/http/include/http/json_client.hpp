#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include "core/script/variant.hpp"
#include "http/client.hpp"
#include "http/method.hpp"

namespace fetch {
namespace http {

class JsonHttpClient
{
public:
  using Variant        = script::Variant;
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  explicit JsonHttpClient(std::string host, uint16_t port = 80);
  ~JsonHttpClient() = default;

  bool Get(ConstByteArray const &endpoint, Variant const &request, Variant &response);
  bool Get(ConstByteArray const &endpoint, Variant &response);
  bool Post(ConstByteArray const &endpoint, Variant const &request, Variant &response);
  bool Post(ConstByteArray const &endpoint, Variant &response);

private:
  bool Request(Method method, ConstByteArray const &endpoint, Variant const *request,
               Variant &response);

  HTTPClient client_;
};

}  // namespace http
}  // namespace fetch
