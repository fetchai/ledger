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

#include "http/validators.hpp"

namespace fetch {
namespace http {
namespace validators {

Validator StringValue(uint16_t min_length, uint16_t max_length)
{
  Validator x;
  x.validator           = [](byte_array::ConstByteArray) { return true; };
  x.schema              = variant::Variant::Object();
  x.schema["type"]      = "string";
  x.schema["minLength"] = min_length;
  x.schema["maxLength"] = max_length;
  return x;
}

}  // namespace validators
}  // namespace http
}  // namespace fetch
