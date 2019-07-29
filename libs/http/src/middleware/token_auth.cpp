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

HTTPServer::RequestMiddleware TokenAuth(byte_array::ConstByteArray const& token, uint32_t level, std::string token_name)
{
  // return the handler
  for(auto &c: token_name)
  {
    c = static_cast<char>(std::tolower(c));
  }

  return [token, token_name, level](HTTPRequest &req) 
  { 
    auto header = req.header();

    if(header.Has(token_name))
    {
      if(header[token_name] == token)
      {
        req.AddAuthentication(token_name, level);
      }
    }
  };
}

}  // namespace middleware
}  // namespace http
}  // namespace fetch
