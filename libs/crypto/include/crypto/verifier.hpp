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

namespace fetch {
namespace crypto {

bool Verify(byte_array::ConstByteArray identity, byte_array::ConstByteArray const &data, byte_array::ConstByteArray const &signature);

class Verifier
{
public:
  using byte_array_type = byte_array::ConstByteArray;
  
  virtual ~Verifier() {}

  virtual Identity identity()                                                            = 0;
  virtual bool     Verify(byte_array_type const &data, byte_array_type const &signature) = 0;
};

}  // namespace crypto
}  // namespace fetch
