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
#include "crypto/identity.hpp"

namespace fetch {
namespace crypto {

class Prover
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  Prover()          = default;
  virtual ~Prover() = default;

  /// @name Prover Interface
  /// @{

  /**
   * Access the corresponding identity for this prover
   *
   * @return The prover
   */
  virtual Identity identity() const = 0;

  /**
   * Load the private key from a byte stream
   */
  virtual void Load(ConstByteArray const &) = 0;

  /**
   * Sign a given message with the
   *
   * @param message The message to be signed
   * @return The generated signature if successful, otherwise return empty byte array
   */
  virtual ConstByteArray Sign(ConstByteArray const &message) = 0;

  /// @}
};

}  // namespace crypto
}  // namespace fetch
