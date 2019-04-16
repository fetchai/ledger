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

#include "crypto/identity.hpp"

#include <memory>

namespace fetch {
namespace crypto {

bool Verify(byte_array::ConstByteArray identity, byte_array::ConstByteArray const &data,
            byte_array::ConstByteArray const &signature);

class Verifier
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  static std::unique_ptr<Verifier> Build(Identity const &identity);
  static bool Verify(Identity const &identity, ConstByteArray const &data, ConstByteArray const &signature);

  Verifier() = default;
  virtual ~Verifier() = default;

  virtual Identity identity()                                                          = 0;
  virtual bool     Verify(ConstByteArray const &data, ConstByteArray const &signature) = 0;
};

}  // namespace crypto
}  // namespace fetch
