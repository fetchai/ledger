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
#include "meta/type_traits.hpp"
#include "vectorise/uint/uint.hpp"

namespace fetch {
namespace crypto {

class StreamHasher
{
public:
  template <uint16_t S>
  using UInt                  = vectorise::UInt<S>;
  virtual void        Reset() = 0;
  virtual bool        Update(uint8_t const *data_to_hash, std::size_t const &size) = 0;
  virtual void        Final(uint8_t *hash, std::size_t const &size)                = 0;
  virtual std::size_t GetSizeInBytes() const                                       = 0;

  bool                  Update(byte_array::ConstByteArray const &data);
  byte_array::ByteArray Final();

  template <typename T>
  meta::IfIsPod<T, T> Final()
  {
    typename std::remove_const<T>::type pod;
    Final(reinterpret_cast<uint8_t *>(&pod), sizeof(pod));
    return pod;
  }

  bool Update(std::string const &str)
  {
    return Update(reinterpret_cast<uint8_t const *>(str.data()), str.size());
  }

  template <typename T>
  meta::IfIsPod<T, bool> Update(T const &pod)
  {
    return Update(reinterpret_cast<uint8_t const *>(&pod), sizeof(pod));
  }

  template <typename T>
  meta::IfIsPod<T, bool> Update(std::vector<T> const &vect)
  {
    return Update(reinterpret_cast<uint8_t const *>(vect.data()), vect.size() * sizeof(T));
  }

  template <uint16_t S>
  UInt<S> Update(UInt<S> const &val)
  {
    return Update(val.pointer(), val.size());
  }

  virtual ~StreamHasher() = default;
};

}  // namespace crypto
}  // namespace fetch
