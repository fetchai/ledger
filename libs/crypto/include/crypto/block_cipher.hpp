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

#include <cstddef>

namespace fetch {
namespace byte_array {

class ConstByteArray;

}  // namespace byte_array

namespace crypto {

class BlockCipher
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  enum Type
  {
    AES_256_CBC,
  };

  /// @name Simple small payload length interface
  /// @{
  static std::size_t GetKeyLength(Type type) noexcept;
  static std::size_t GetIVLength(Type type) noexcept;
  static bool        Encrypt(Type type, ConstByteArray const &key, ConstByteArray const &iv,
                             ConstByteArray const &clear_text, ConstByteArray &cipher_text);
  static bool        Decrypt(Type type, ConstByteArray const &key, ConstByteArray const &iv,
                             ConstByteArray const &cipher_text, ConstByteArray &clear_text);
  /// @}
};

}  // namespace crypto
}  // namespace fetch
