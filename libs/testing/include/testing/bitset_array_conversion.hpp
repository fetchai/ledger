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
#include "meta/log2.hpp"

#include <array>
#include <bitset>
#include <iomanip>
#include <ostream>

namespace fetch {
namespace testing {

template <typename T, std::size_t SIZE, std::size_t BITS = (SIZE * sizeof(T) * 8),
          meta::EnableIf<meta::IsInteger<T> && meta::IsLog2(BITS)> * = nullptr>
using Array = std::array<T, SIZE>;

template <typename T, std::size_t BITS>
using ArraySizeInBits = Array<T, (BITS >> meta::Log2(sizeof(T) * 8))>;

template <typename T, std::size_t BITS>
ArraySizeInBits<T, BITS> to_array(std::bitset<BITS> const &bs)
{
  constexpr std::size_t T_BITS_LOG2{meta::Log2(sizeof(T) * 8)};
  constexpr T           T_BITS_MASK{(1 << T_BITS_LOG2) - 1};

  ArraySizeInBits<T, BITS> to;
  to.fill(static_cast<T>(0));
  for (std::size_t i = 0; i < bs.size(); ++i)
  {
    auto const &bit_value = bs[i];
    if (bit_value)
    {
      auto &segment = to[i >> T_BITS_LOG2];
      segment |= static_cast<T>(static_cast<T>(1) << (i & T_BITS_MASK));
    }
  }
  return to;
}

template <typename T, std::size_t SIZE, std::size_t BITS = (SIZE * sizeof(T) * 8)>
std::bitset<BITS> to_bitset(Array<T, SIZE> const &from)
{
  constexpr std::size_t T_BITS_LOG2{meta::Log2(sizeof(T) * 8)};
  constexpr T           T_BITS_MASK{(1 << T_BITS_LOG2) - 1};

  std::bitset<BITS> bs;
  bs.reset();
  for (std::size_t i = 0; i < bs.size(); ++i)
  {
    auto const &segment   = from[i >> T_BITS_LOG2];
    auto const  bit_value = segment & static_cast<T>(static_cast<T>(1) << (i & T_BITS_MASK));

    if (bit_value)
    {
      bs[i] = true;
    }
  }
  return bs;
}

template <typename T, std::size_t BITS>
meta::EnableIf<meta::IsInteger<T> && meta::IsLog2(BITS), byte_array::ByteArray> to_ByteArray(
    ArraySizeInBits<T, BITS> const &from)
{
  return {reinterpret_cast<byte_array::ConstByteArray::container_type *>(from.data()),
          from.size() * sizeof(decltype(from)::value_type)};
}

template <std::size_t BITS>
meta::EnableIf<meta::IsLog2(BITS), byte_array::ByteArray> to_ByteArray(
    std::bitset<BITS> const &from)
{
  using T = byte_array::ConstByteArray::container_type;
  auto const arr{to_array<T>(from)};
  return {reinterpret_cast<T const *const>(arr.data()), arr.size() * sizeof(T)};
}


template <typename T, std::size_t SIZE>
meta::EnableIf<meta::IsInteger<T>, std::ostream &> operator<<(std::ostream &      ostream,
                                                              std::array<T, SIZE> arr)
{
  ostream << to_ByteArray(arr).ToHex();
  return ostream;
}

}  // namespace testing
}  // namespace fetch
