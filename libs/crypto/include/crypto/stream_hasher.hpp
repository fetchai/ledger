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

template <typename Derived>
class StreamHasher
{
public:
  bool Update(byte_array::ConstByteArray const &data)
  {
    return derived().UpdateHasher(data.pointer(), data.size());
  }

  byte_array::ByteArray Final()
  {
    byte_array::ByteArray digest;
    digest.Resize(Derived::size_in_bytes);
    derived().FinalHasher(digest.pointer(), digest.size());

    return digest;
  }

  bool Update(uint8_t const *data_to_hash, std::size_t size)
  {
    return derived().UpdateHasher(data_to_hash, size);
  }

  void Final(uint8_t *hash, std::size_t size)
  {
    derived().FinalHasher(hash, size);
  }

  void Reset()
  {
    derived().ResetHasher();
  }

  bool Update(std::string const &str)
  {
    return derived().UpdateHasher(reinterpret_cast<uint8_t const *>(str.data()), str.size());
  }

  //  template <typename T>
  //  meta::IfIsPod<T, T> Final()
  //  {
  //    typename std::remove_const<T>::type pod;
  //    derived().FinalHasher(reinterpret_cast<uint8_t *>(&pod), sizeof(pod));
  //    return pod;
  //  }

  //  template <typename T>
  //  meta::IfIsPod<T, bool> Update(T const &pod)
  //  {
  //    return derived().UpdateHasher(reinterpret_cast<uint8_t const *>(&pod), sizeof(pod));
  //  }

  //  template <typename T>
  //  meta::IfIsPod<T, bool> Update(std::vector<T> const &vect)
  //  {
  //    return derived().UpdateHasher(reinterpret_cast<uint8_t const *>(vect.data()), vect.size() *
  //    sizeof(T));
  //  }

private:
  constexpr Derived &derived()
  {
    return *static_cast<Derived *>(this);
  }
};

}  // namespace crypto
}  // namespace fetch
