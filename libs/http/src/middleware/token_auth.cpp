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

#include "http/authentication_level.hpp"
#include "http/middleware/token_auth.hpp"

#include <cstdint>
#include <utility>

namespace fetch {
namespace http {
namespace middleware {

void TokenAuthenticationInterface::operator()(HTTPRequest &req)
{
  byte_array::ConstByteArray token_name{"authorization"};
  auto                       header = req.header();

  if (header.Has(token_name))
  {
    auto token = header[token_name];
    if (!token.Match("Token "))
    {
      return;
    }

    token      = token.SubArray(6);
    auto level = ValidateToken(token);
    if (level != AuthenticationLevel::NO_ACCESS)
    {
      req.AddAuthentication(token_name, level);
    }
  }
}

SimpleTokenAuthentication::SimpleTokenAuthentication(ConstByteArray token,
                                                     uint32_t       authentication_level)
  : token_{std::move(token)}
  , authentication_level_{authentication_level}
{}

uint32_t SimpleTokenAuthentication::ValidateToken(byte_array::ConstByteArray const &token)
{
  return (token_ == token) ? authentication_level_
                           : static_cast<uint32_t>(AuthenticationLevel::NO_ACCESS);
}

}  // namespace middleware
}  // namespace http
}  // namespace fetch
