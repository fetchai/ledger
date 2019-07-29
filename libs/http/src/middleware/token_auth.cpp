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

#include "core/logging.hpp"
#include "http/middleware/token_auth.hpp"

#include <memory>

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

    token = token.SubArray(6);
    if (ValidateToken(token))
    {
      req.AddAuthentication(token_name, authentication_level());
    }
  }
};

bool SimpleTokenAuthentication::ValidateToken(byte_array::ConstByteArray const &token)
{
  return token_ == token;
}

uint32_t SimpleTokenAuthentication::authentication_level() const
{
  return authentication_level_;
}

}  // namespace middleware
}  // namespace http
}  // namespace fetch
