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

#include "http/json_response.hpp"
#include "http/module.hpp"
#include "http/server.hpp"
#include "logging/logging.hpp"

#include <mutex>

namespace fetch {
namespace constellation {

class OpenAPIHttpModule : public http::HTTPModule
{
public:
  OpenAPIHttpModule();
  void Reset(http::HTTPServer *srv = nullptr);

private:
  using Variant        = variant::Variant;
  using ConstByteArray = byte_array::ConstByteArray;

  http::HTTPServer *server_{nullptr};
  Mutex             server_lock_;
};

}  // namespace constellation
}  // namespace fetch
