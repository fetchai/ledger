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

#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace byte_array {

ConstByteArray ToBase64(ConstByteArray const &str);
ConstByteArray ToHex(ConstByteArray const &str);
ConstByteArray ToBin(ConstByteArray const &str);
ConstByteArray ToHexReverse(ConstByteArray const &str);
ConstByteArray ToBinReverse(ConstByteArray const &str);
ConstByteArray ToHumanReadable(ConstByteArray const &str);
}  // namespace byte_array
}  // namespace fetch
