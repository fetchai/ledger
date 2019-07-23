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

#include "core/serializers/group_definitions.hpp"
#include "vectorise/memory/shared_array.hpp"
#include "vectorise/platform.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>

namespace fetch {

class BitVector
{
public:
  using Block           = uint64_t;
  using UnderlyingArray = memory::SharedArray<Block>;

  enum
  {
    ELEMENT_BIT_SIZE = sizeof(Block) << 3u,
    LOG_BITS         = meta::Log2(ELEMENT_BIT_SIZE),
    BIT_MASK         = (1ull << LOG_BITS) - 1,
    SIMD_SIZE        = UnderlyingArray::E_SIMD_COUNT
  };

  // Construction  / Destruction
  explicit BitVector(std::size_t n = 0);
  BitVector(BitVector const &other);
  ~BitVector() = default;

  void Resize(std::size_t bit_size);

  void SetAllZero();
  void SetAllOne();

  bool RemapTo(BitVector &dst) const;

  static bool Expand(BitVector const &src, BitVector &dst);
  static bool Contract(BitVector const &src, BitVector &dst);

  void InlineAndAssign(BitVector const &a, BitVector const &b);

  std::size_t            size() const;
  uint32_t               log2_size() const;
  std::size_t            blocks() const;
  UnderlyingArray const &data() const;
  UnderlyingArray &      data();

  std::size_t PopCount() const;

  void conditional_flip(std::size_t block, std::size_t bit, uint64_t base);
  void conditional_flip(std::size_t bit, uint64_t base);

  void flip(std::size_t block, std::size_t bit);
  void flip(std::size_t bit);

  Block bit(std::size_t block, std::size_t b) const;
  Block bit(std::size_t b) const;

  void set(std::size_t block, std::size_t bit, uint64_t val);
  void set(std::size_t bit, uint64_t val);

  Block &      operator()(std::size_t n);
  Block const &operator()(std::size_t n) const;

  bool operator==(BitVector const &other) const;
  bool operator!=(BitVector const &other) const;

  BitVector &operator^=(BitVector const &other);
  BitVector &operator&=(BitVector const &other);
  BitVector &operator|=(BitVector const &other);

  BitVector operator^(BitVector const &other) const;
  BitVector operator&(BitVector const &other) const;
  BitVector operator|(BitVector const &other) const;

private:
  UnderlyingArray data_{};
  std::size_t     size_{0};
  std::size_t     blocks_{0};
};

inline std::ostream &operator<<(std::ostream &s, BitVector const &b)
{
#if 1
  for (std::size_t i = 0; i < b.size(); ++i)
  {
    if (i && ((i % 10) == 0))
    {
      s << ' ';
    }
    s << b.bit(i);
  }
#else
  for (std::size_t i = 0; i < b.blocks(); ++i)
  {
    if (i != 0)
    {
      s << " ";
    }
    s << std::hex << b(i);
  }
#endif

  return s;
}

namespace serializers {

template <typename D>
struct ArraySerializer<BitVector, D>
{
public:
  using Type       = BitVector;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &mask)
  {
    uint64_t const bit_size   = mask.size();
    uint64_t const block_size = mask.blocks();

    auto array = array_constructor(block_size + 1);

    auto const &underlying_blocks = mask.data();
    array.Append(bit_size);
    for (uint64_t i = 0; i < block_size; ++i)
    {
      array.Append(underlying_blocks[i]);
    }
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &mask)
  {
    uint64_t bit_size   = 0;
    uint64_t block_size = array.size() - 1;

    array.GetNextValue(bit_size);

    mask.Resize(bit_size);
    assert(mask.blocks() == block_size);
    auto &underlying_blocks = mask.data();

    for (uint64_t i = 0; i < block_size; ++i)
    {
      array.GetNextValue(underlying_blocks[i]);
    }
  }
};

}  // namespace serializers
}  // namespace fetch
