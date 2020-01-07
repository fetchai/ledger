#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "http/authentication_level.hpp"
#include "http/server.hpp"

#include <cstdint>
#include <utility>

namespace fetch {
namespace http {
namespace middleware {

class TokenAuthenticationInterface
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  TokenAuthenticationInterface()          = default;
  virtual ~TokenAuthenticationInterface() = default;

  virtual uint32_t ValidateToken(ConstByteArray const &token) = 0;

  void operator()(HTTPRequest &req);
};

class SimpleTokenAuthentication : public TokenAuthenticationInterface
{
public:
  explicit SimpleTokenAuthentication(
      ConstByteArray token, uint32_t authentication_level = AuthenticationLevel::TOKEN_PRESENT);

  uint32_t ValidateToken(ConstByteArray const &token) override;

private:
  ConstByteArray token_;
  uint32_t       authentication_level_;
};

}  // namespace middleware
}  // namespace http
}  // namespace fetch
