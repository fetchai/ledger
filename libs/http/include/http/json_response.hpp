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

#include "http/response.hpp"
#include "http/status.hpp"

namespace fetch {

namespace byte_array {
class ConstByteArray;
}
namespace variant {
class Variant;
}

namespace http {

http::HTTPResponse CreateJsonResponse(byte_array::ConstByteArray const &body,
                                      Status status = Status::SUCCESS_OK);

http::HTTPResponse CreateJsonResponse(variant::Variant const &doc,
                                      Status                  status = Status::SUCCESS_OK);

}  // namespace http
}  // namespace fetch
