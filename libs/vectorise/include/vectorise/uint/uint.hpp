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
#include "meta/has_index.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace fetch {
namespace vectorise {
/* Implements a subset of big number functionality.
 *
 * The purpose of this library is to implement a subset of number
 * functionalities that is handy when for instance computing
 * proof-of-work or other big uint manipulations.
 *
 * The implementation subclasses a <byte_array::ConstByteArray> such
 * one easily use this in combination with hashes etc.
 */
template <uint16_t S = 256>
class UInt
{
public:
  using BaseType = uint8_t;
  enum
  {
    UINT_SIZE = S,
    ELEMENTS  = UINT_SIZE / sizeof(BaseType) / 8
  };
  static_assert(S == (ELEMENTS * sizeof(BaseType) << 3),
                "Size must be a multiple of 8 times the base type size.");

  using ContainerType                       = std::array<uint8_t, ELEMENTS>;
  static constexpr char const *LOGGING_NAME = "UInt";

  UInt()
  {
    for (std::size_t i = 0; i < ELEMENTS; ++i)
    {
      data_[i] = 0;
    }
  }

  UInt(UInt const &other)
    : data_{other.data_}
  {}

  UInt(ContainerType const &other)
    : data_(other)
  {}

  template <typename T>
  UInt(T const &other)
  {
    for (std::size_t i = 0; i < ELEMENTS; ++i)
    {
      data_[i] = 0;
    }

    // TODO: Copy instead
    *this = other;
  }

  UInt(uint64_t const &number)
  {
    union
    {
      uint64_t value;
      uint8_t  bytes[sizeof(uint64_t) / sizeof(BaseType)];
    } data;
    data.value = number;

    // FIXME: Assumes little endian
    std::size_t i = 0;
    for (; i < sizeof(uint64_t); ++i)
    {
      data_[i] = data.bytes[i];
    }
    for (; i < ELEMENTS; ++i)
    {
      data_[i] = 0;
    }
  }

  bool operator==(UInt const &other) const
  {
    return data_ == other.data_;
  }

  UInt &operator=(UInt const &v)
  {
    data_ = v.data_;
    return *this;
  }

  template <typename ArrayType>
  meta::HasIndex<ArrayType, UInt> &operator=(ArrayType const &v)
  {
    // TODO: Assert that index return type is the same size
    if (data_.size() < v.size())
    {
      throw std::runtime_error("Array size is greater than UInt capacity");
    }

    uint64_t i = 0;
    for (; i < v.size(); ++i)
    {
      data_[i] = static_cast<BaseType>(v[i]);
    }

    for (; i < data_.size(); ++i)
    {
      data_[i] = 0;
    }

    return *this;
  }

  template <typename T>
  meta::HasNoIndex<T, UInt> &operator=(T const &v)
  {
    union
    {
      uint64_t value;
      uint8_t  bytes[sizeof(T)];
    } data;
    data.value = uint64_t(v);

    if (sizeof(T) > data_.size())
    {
      throw std::runtime_error("UInt cannot contain object with more then 256 bits/");
    }

    // FIXME: Assumes little endian
    std::size_t i = 0;
    for (; i < sizeof(T); ++i)
    {
      data_[i] = data.bytes[i];
    }

    for (; i < data_.size(); ++i)
    {
      data_[i] = 0;
    }
    return *this;
  }

  UInt &operator++()
  {
    // TODO(issue 32): Propagation of carry bits this way is slow.
    // If we instead make sure that the size is always a multiple of 8
    // We can do the logic in uint64_t
    // In fact, we should re implement this using vector registers.
    std::size_t i   = 0;
    uint8_t     val = ++data_[i];
    while (val == 0)
    {
      ++i;
      if (i == data_.size())
      {
        throw std::runtime_error("Throw error, size too little");
        val = data_[i] = 1;
      }
      else
      {
        val = ++data_[i];
      }
    }

    return *this;
  }

  UInt &operator<<=(std::size_t const &n)
  {
    std::size_t bits  = n & 7;
    std::size_t bytes = n >> 3;
    std::size_t old   = data_.size() - bytes;

    for (std::size_t i = old; i != 0;)
    {
      --i;
      data_[i + bytes] = data_[i];
    }

    for (std::size_t i = 0; i < bytes; ++i)
    {
      data_[i] = 0;
    }

    std::size_t nbits = 8 - bits;
    uint8_t     carry = 0;
    for (std::size_t i = 0; i < data_.size(); ++i)
    {
      uint8_t val = data_[i];

      data_[i] = uint8_t(uint8_t(val << bits) | carry);
      carry    = uint8_t(val >> nbits);
    }

    return *this;
  }

  uint8_t operator[](std::size_t const &n) const
  {
    return data_[n];
  }

  bool operator<(UInt const &other) const
  {
    uint64_t s1 = TrimmedSize();
    uint64_t s2 = other.TrimmedSize();

    if (s1 != s2)
    {
      return s1 < s2;
    }
    if (s1 == 0)
    {
      return false;
    }

    --s1;
    while ((s1 != 0) && (data_[s1] == other[s1]))
    {
      --s1;
    }

    return data_[s1] < other[s1];
  }

  bool operator>(UInt const &other) const
  {
    return other < (*this);
  }

  uint64_t TrimmedSize() const
  {
    uint64_t ret = data_.size();
    while ((ret != 0) && (data_[ret - 1] == 0))
    {
      --ret;
    }
    return ret;
  }

  uint8_t const *pointer() const
  {
    return data_.data();
  }

  uint64_t size() const
  {
    return data_.size();
  }

  template <typename T, uint16_t G>
  friend void Serialize(T &s, UInt<G> const &u);

  template <typename T, uint16_t G>
  friend void Deserialize(T &s, UInt<G> &u);

private:
  ContainerType data_;
};

inline double Log(UInt<256> const &x)
{
  uint64_t last_byte = x.TrimmedSize();
  union
  {
    uint8_t  bytes[4];
    uint32_t value;
  } fraction;

  assert(last_byte >= 4);

  std::size_t j = last_byte - 4;

  fraction.bytes[0] = x[j];
  fraction.bytes[1] = x[j + 1];
  fraction.bytes[2] = x[j + 2];
  fraction.bytes[3] = x[j + 3];

  assert(fraction.value != 0);

  uint64_t tz       = uint64_t(__builtin_ctz(fraction.value));
  uint64_t exponent = (last_byte << 3) - tz;

  return double(exponent) + std::log(double(fraction.value << tz) * (1. / double(uint32_t(-1))));
}

inline double ToDouble(UInt<256> const &x)
{
  uint64_t last_byte = x.TrimmedSize();

  union
  {
    uint8_t  bytes[4];
    uint32_t value;
  } fraction;

  assert(last_byte >= 4);

  std::size_t j = last_byte - 4;

  fraction.bytes[0] = x[j];
  fraction.bytes[1] = x[j + 1];
  fraction.bytes[2] = x[j + 2];
  fraction.bytes[3] = x[j + 3];

  assert(fraction.value != 0);
  uint16_t tz       = uint16_t(__builtin_ctz(
      fraction.value));  // TODO(issue 31): Wrap in function for cross compiler portability
  uint16_t exponent = uint16_t((last_byte << 3) - tz);

  assert(exponent < 1023);

  union
  {
    double   value;
    uint64_t bits;
  } conv;

  conv.bits = 0;
  conv.bits |= uint64_t(uint64_t(exponent & ((1 << 12) - 1)) << 52);
  conv.bits |= uint64_t((fraction.value << (20 + tz)) & ((1ull << 53) - 1));
  return conv.value;
}

template <typename T, uint16_t S>
void Serialize(T &s, UInt<S> const &u)
{
  s << u.data_;
}

template <typename T, uint16_t S>
void Deserialize(T &s, UInt<S> &u)
{
  s >> u.data_;
}

}  // namespace vectorise
}  // namespace fetch
