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
  using byte_array_type = byte_array::ConstByteArray;

  virtual Identity identity() const = 0;
  virtual ~Prover()
  {}

  virtual void            Load(byte_array_type const &)     = 0;
  virtual bool            Sign(byte_array_type const &text) = 0;
  virtual byte_array_type document_hash()                   = 0;
  virtual byte_array_type signature()                       = 0;
};

}  // namespace crypto
}  // namespace fetch
