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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/hasher_interface.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace fetch {
namespace crypto {

template <typename T>
byte_array::ByteArray Hash(byte_array::ConstByteArray const &input)
{
  static_assert(std::is_base_of<HasherInterface, T>::value,
                "Use a type derived from fetch::crypto::Hasher:Interface");
  T hasher;

  hasher.Reset();
  hasher.Update(input);
  return hasher.Final();
}

template <typename T>
void Hash(uint8_t const *const input, std::size_t const input_size, uint8_t *const output)
{
  static_assert(std::is_base_of<HasherInterface, T>::value,
                "Use a type derived from fetch::crypto::Hasher:Interface");
  T hasher;

  hasher.Reset();
  hasher.Update(input, input_size);
  hasher.Final(output);
}

}  // namespace crypto
}  // namespace fetch
