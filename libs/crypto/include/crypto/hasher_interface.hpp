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
#include "core/macros.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/uint/uint.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {
namespace internal {

/*
 * CRTP-based interface for digest contexts. Implementing classes should define the following:
 *
 * ```
 *   static constexpr std::size_t size_in_bytes;
 *   bool ResetHasherInternal();
 *   bool UpdateHasherInternal(uint8_t const *data_to_hash, std::size_t size);
 *   bool FinalHasherInternal(uint8_t *hash, std::size_t size);
 * ```
 *
 * The `size_in_bytes` constant should be equal to the size of the hash (e.g. 32u for SHA256).
 * The `*Internal` methods should return `true` if their respective operation was successful,
 * `false` otherwise.
 *
 */
template <typename Derived>
class HasherInterface
{
public:
  HasherInterface()                        = default;
  ~HasherInterface()                       = default;
  HasherInterface(HasherInterface const &) = delete;
  HasherInterface(HasherInterface &&)      = delete;

  HasherInterface &operator=(HasherInterface const &) = delete;
  HasherInterface &operator=(HasherInterface &&) = delete;

  // Direct call-through methods to Derived
  void Reset()
  {
    auto const success = derived().ResetHasherInternal();
    FETCH_UNUSED(success);
    assert(success);
  }

  bool Update(uint8_t const *data_to_hash, std::size_t size)
  {
    auto const success = derived().UpdateHasherInternal(data_to_hash, size);
    assert(success);

    return success;
  }

  void Final(uint8_t *const hash)
  {
    auto const success = derived().FinalHasherInternal(hash);
    FETCH_UNUSED(success);
    assert(success);
  }

  // Convenience methods

  bool Update(byte_array::ConstByteArray const &data)
  {
    auto const success = derived().UpdateHasherInternal(data.pointer(), data.size());
    assert(success);

    return success;
  }

  bool Update(std::string const &str)
  {
    auto const success =
        derived().UpdateHasherInternal(reinterpret_cast<uint8_t const *>(str.data()), str.size());
    assert(success);

    return success;
  }

  byte_array::ByteArray Final()
  {
    byte_array::ByteArray digest;
    digest.Resize(Derived::size_in_bytes);

    auto const success = derived().FinalHasherInternal(digest.pointer());
    FETCH_UNUSED(success);
    assert(success);

    return digest;
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
