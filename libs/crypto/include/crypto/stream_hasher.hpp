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

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {
namespace internal {

template <typename Derived>
class StreamHasher
{
public:
  StreamHasher()                     = default;
  ~StreamHasher()                    = default;
  StreamHasher(StreamHasher const &) = delete;
  StreamHasher(StreamHasher &&)      = delete;

  StreamHasher &operator=(StreamHasher const &) = delete;
  StreamHasher &operator=(StreamHasher &&) = delete;

  void Reset()
  {
    auto const success = derived().ResetHasher();
    assert(success);
  }

  bool Update(uint8_t const *data_to_hash, std::size_t size)
  {
    auto const success = derived().UpdateHasher(data_to_hash, size);
    assert(success);

    return success;
  }

  bool Update(byte_array::ConstByteArray const &data)
  {
    auto const success = derived().UpdateHasher(data.pointer(), data.size());
    assert(success);

    return success;
  }

  bool Update(std::string const &str)
  {
    auto const success =
        derived().UpdateHasher(reinterpret_cast<uint8_t const *>(str.data()), str.size());
    assert(success);

    return success;
  }

  byte_array::ByteArray Final()
  {
    byte_array::ByteArray digest;
    digest.Resize(Derived::size_in_bytes);

    auto const success = derived().FinalHasher(digest.pointer(), digest.size());
    assert(success);

    return digest;
  }

  void Final(uint8_t *hash, std::size_t size)
  {
    auto const success = derived().FinalHasher(hash, size);
    assert(success);
  }

private:
  constexpr Derived &derived()
  {
    return *static_cast<Derived *>(this);
  }
};

}  // namespace internal
}  // namespace crypto
}  // namespace fetch
