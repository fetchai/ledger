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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"

namespace fetch {
namespace byte_array {

ConstByteArray ConstByteArray::ToBase64() const
{
  return fetch::byte_array::ToBase64(*this);
}

ConstByteArray ConstByteArray::ToHex() const
{
  return fetch::byte_array::ToHex(*this);
}

std::ostream &operator<<(std::ostream &os, ConstByteArray const &str)
{
  auto const *arr = reinterpret_cast<char const *>(str.pointer());
  for (std::size_t i = 0; i < str.size(); ++i)
  {
    os << arr[i];
  }
  return os;
}

ConstByteArray operator+(char const *a, ConstByteArray const &b)
{
  ConstByteArray s(a);
  s = s + b;
  return s;
}

ConstByteArray ConstByteArray::FromBase64() const
{
  return fetch::byte_array::FromBase64(*this);
}

ConstByteArray ConstByteArray::FromHex() const
{
  return fetch::byte_array::FromHex(*this);
}

}  // namespace byte_array
}  // namespace fetch
