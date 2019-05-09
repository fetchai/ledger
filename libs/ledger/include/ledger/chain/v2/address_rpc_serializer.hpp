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

#include "ledger/chain/v2/address.hpp"

namespace fetch {
namespace ledger {
namespace v2 {

template <typename T>
void Serialize(T &s, Address const &address)
{
  s << address.address();
}

template <typename T>
void Deserialize(T &s, Address &address)
{
  // extract the data from the stream
  byte_array::ConstByteArray data;
  s >> data;

  // create the address
  address = Address{data};
}

} // namespace v2
} // namespace ledger
} // namespace fetch
