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
#include "http/authentication_level.hpp"
#include "http/middleware/token_auth.hpp"

#include <memory>

namespace fetch {
namespace http {
namespace middleware {

HTTPServer::RequestMiddleware DenyAll()
{
  // return the handler
  return [](HTTPRequest &req) { req.SetAuthentication("", AuthenticationLevel::NO_ACCESS); };
}

}  // namespace middleware
}  // namespace http
}  // namespace fetch
