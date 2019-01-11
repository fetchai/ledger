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

namespace fetch {
namespace byte_array {

class ConstByteArray;

}  // namespace byte_array
namespace http {

enum class Method
{
  GET     = 1,
  POST    = 2,
  PUT     = 3,
  PATCH   = 4,
  DELETE  = 5,
  OPTIONS = 6
};

char const *ToString(Method method);
bool        FromString(byte_array::ConstByteArray const &text, Method &method);

}  // namespace http
}  // namespace fetch
