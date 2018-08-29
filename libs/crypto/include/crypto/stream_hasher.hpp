#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
namespace crypto {

class StreamHasherLowLevel
{
public:
  virtual ~StreamHasherLowLevel() {}

  virtual std::size_t hashSize() const                                             = 0;
  virtual void        Reset()                                                      = 0;
  virtual bool        Update(uint8_t const *data_to_hash, std::size_t const &size) = 0;
  virtual void        Final(uint8_t *hash, std::size_t const &size)                = 0;
};

class StreamHasher : public virtual StreamHasherLowLevel
{
public:
  using byte_array_type = byte_array::ByteArray;

  using StreamHasherLowLevel::Update;
  using StreamHasherLowLevel::Final;

  virtual bool            Update(byte_array_type const &data) = 0;
  virtual void            Final()                             = 0;
  virtual byte_array_type digest() const                      = 0;
};

}  // namespace crypto
}  // namespace fetch
