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

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {

class HasherInterface
{
public:
  HasherInterface()                        = default;
  virtual ~HasherInterface()               = default;
  HasherInterface(HasherInterface const &) = delete;
  HasherInterface(HasherInterface &&)      = delete;

  HasherInterface &operator=(HasherInterface const &) = delete;
  HasherInterface &operator=(HasherInterface &&) = delete;

  // Direct call-through methods to Derived
  virtual void        Reset()                                               = 0;
  virtual bool        Update(uint8_t const *data_to_hash, std::size_t size) = 0;
  virtual void        Final(uint8_t *hash)                                  = 0;
  virtual std::size_t HashSizeInBytes() const                               = 0;

  // Convenience methods
  bool Update(std::string const &str)
  {
    return Update(reinterpret_cast<uint8_t const *>(str.data()), str.size());
  }

  bool Update(byte_array::ConstByteArray const &s)
  {
    return Update(s.pointer(), s.size());
  }

  byte_array::ByteArray Final()
  {
    byte_array::ByteArray digest;
    digest.Resize(HashSizeInBytes());
    Final(digest.pointer());

    return digest;
  }
};

}  // namespace crypto
}  // namespace fetch
