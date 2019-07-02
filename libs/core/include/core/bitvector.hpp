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

#include "vectorise/memory/shared_array.hpp"
#include "vectorise/platform.hpp"
#include "core/serializers/group_definitions.hpp"

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

inline BitVector::BitVector(std::size_t n)
{
  Resize(n);
}

inline BitVector::BitVector(BitVector const &other)
  : data_(other.data_.Copy())
  , size_(other.size_)
  , blocks_(other.blocks_)
{}

/**
 * Resize the vector to n bits
 *
 * @param bit_size The size in bits of the vector
 */
inline void BitVector::Resize(std::size_t bit_size)
{
  UnderlyingArray old = data_;

  // calculate
  std::size_t const num_elements = (bit_size + (ELEMENT_BIT_SIZE - 1)) / ELEMENT_BIT_SIZE;

  data_   = UnderlyingArray(num_elements);
  blocks_ = num_elements;
  size_   = bit_size;

  if (bit_size > 0)
  {
    SetAllZero();  // TODO(issue 29): Only set those
  }

  // TODO(issue 29): Copy data;
}

inline void BitVector::SetAllZero()
{
  data_.SetAllZero();
}

inline void BitVector::SetAllOne()
{
  auto *buffer = reinterpret_cast<uint8_t *>(data_.pointer());
  std::memset(buffer, 0xFF, data_.size() * sizeof(Block));
}

inline bool BitVector::RemapTo(BitVector &dst) const
{
  if (dst.size() >= size())
  {
    return Expand(*this, dst);
  }
  else
  {
    return Contract(*this, dst);
  }
}

inline bool BitVector::Expand(BitVector const &src, BitVector &dst)
{
  using BitVectorPtr = std::unique_ptr<BitVector>;

  auto const current_size = static_cast<uint32_t>(src.size());
  auto const next_size    = static_cast<uint32_t>(dst.size());

  // early exit in the case where the source bit vector is 1 or 0 (wildcard)
  if (src.size() <= 1)
  {
    dst.SetAllOne();
    return true;
  }

  if (next_size < current_size)
  {
    return false;
  }

  // ensure the bit vectors are compatible sizes
  if (!(platform::IsLog2(next_size) && platform::IsLog2(current_size)))
  {
    return false;
  }

  BitVectorPtr intermediate_vector{};

  // determine the number of loops that needs to be performed
  auto const num_loops = platform::ToLog2(next_size) - platform::ToLog2(current_size);

  // define the various pointers to the storage
  auto      src_buffer = reinterpret_cast<uint8_t const *>(src.data().pointer());
  uint16_t *int_buffer = nullptr;
  auto      dst_buffer = reinterpret_cast<uint16_t *>(dst.data().pointer());

  // in cases larger than 1 and additional buffer is required
  if (num_loops > 1)
  {
    // create the intermediate vector
    intermediate_vector = std::make_unique<BitVector>(dst.size());

    // update the intermediate buffer pointer
    int_buffer = reinterpret_cast<uint16_t *>(intermediate_vector->data().pointer());

    // in the case of even number of loops we need to swap the intermediate and destination buffers
    // to ensure the correct final destination
    if ((num_loops & 0x1u) == 0)
    {
      std::swap(int_buffer, dst_buffer);
    }
  }

  uint32_t current_size_bytes = platform::DivCeil<8u>(current_size);
  for (uint32_t i = 0; i < num_loops; ++i)
  {
    // loop over all the bytes of the current input and generate 16bit combinations
    for (std::size_t j = 0; j < current_size_bytes; ++j)
    {
      uint64_t const m =
          ((src_buffer[j] * 0x0101010101010101ULL) & 0x8040201008040201ULL) * 0x0102040810204081ULL;

      dst_buffer[j] = static_cast<uint16_t>(((m >> 49u) & 0x5555u) | ((m >> 48u) & 0xAAAAu));
    }

    // adjust the size in the case of multiple loops
    current_size_bytes *= 2u;

    // the current destination buffer always becomes the next source buffer
    src_buffer = reinterpret_cast<uint8_t *>(dst_buffer);

    // swap the intermediate and destinations buffers ready for the next loop
    std::swap(int_buffer, dst_buffer);
  }

  return true;
}

inline bool BitVector::Contract(BitVector const &src, BitVector &dst)
{
  using BitVectorPtr = std::unique_ptr<BitVector>;

  auto const current_size = static_cast<uint32_t>(src.size());
  auto const next_size    = static_cast<uint32_t>(dst.size());

  // early exit in the case where the destination bit vector is 1 or 0 (wildcard)
  if (dst.size() <= 1)
  {
    dst.SetAllOne();
    return true;
  }

  if (next_size > current_size)
  {
    return false;
  }

  // ensure the bit vectors are compatible sizes
  if (!(platform::IsLog2(next_size) && platform::IsLog2(current_size)))
  {
    return false;
  }

  BitVectorPtr intermediate_vector{};

  // determine the number of loops that needs to be performed
  auto const num_loops = platform::ToLog2(current_size) - platform::ToLog2(next_size);

  // define the various pointers to the storage
  auto     src_buffer = reinterpret_cast<uint16_t const *>(src.data().pointer());
  uint8_t *int_buffer = nullptr;
  auto     dst_buffer = reinterpret_cast<uint8_t *>(dst.data().pointer());

  // in cases larger than 1 and additional buffer is required
  if (num_loops > 1)
  {
    // create the intermediate vector
    intermediate_vector = std::make_unique<BitVector>(dst.size());

    // update the intermediate buffer pointer
    int_buffer = reinterpret_cast<uint8_t *>(intermediate_vector->data().pointer());

    // in the case of even number of loops we need to swap the intermediate and destination buffers
    // to ensure the correct final destination
    if ((num_loops & 0x1u) == 0)
    {
      std::swap(int_buffer, dst_buffer);
    }
  }

  uint32_t current_size_words = platform::DivCeil<16u>(current_size);
  for (uint32_t i = 0; i < num_loops; ++i)
  {
    // loop over all the bytes of the current input and generate 16bit combinations
    for (std::size_t j = 0; j < current_size_words; ++j)
    {
      uint16_t const a = (src_buffer[j] & 0x5555u) | ((src_buffer[j] & 0xAAAAu) >> 1u);
      dst_buffer[j]    = static_cast<uint8_t>(
          (a & 0x1u) | ((a >> 1u) & 0x2u) | ((a >> 2u) & 0x4u) | ((a >> 3u) & 0x8u) |
          ((a >> 4u) & 0x10u) | ((a >> 5u) & 0x20u) | ((a >> 6u) & 0x40u) | ((a >> 7u) & 0x80u));
    }

    // adjust the size in the case of multiple loops
    current_size_words /= 2;

    if (current_size_words == 0)
    {
      current_size_words = 1;
    }

    // the current destination buffer always becomes the next source buffer
    src_buffer = reinterpret_cast<uint16_t *>(dst_buffer);

    // swap the intermediate and destinations buffers ready for the next loop
    std::swap(int_buffer, dst_buffer);
  }

  return true;
}

inline bool BitVector::operator==(BitVector const &other) const
{
  if (size_ != other.size_)
  {
    return false;
  }
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    if (operator()(i) != other(i))
    {
      return false;
    }
  }
  return true;
}

inline bool BitVector::operator!=(BitVector const &other) const
{
  return !operator==(other);
}

inline BitVector &BitVector::operator^=(BitVector const &other)
{
  assert(size_ == other.size_);
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    data_[i] ^= other.data_[i];
  }

  return *this;
}

inline BitVector BitVector::operator^(BitVector const &other) const
{
  assert(size_ == other.size_);
  BitVector ret(*this);
  ret ^= other;
  return ret;
}

inline BitVector &BitVector::operator&=(BitVector const &other)
{
  assert(size_ == other.size_);
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    data_[i] &= other.data_[i];
  }

  return *this;
}

inline BitVector BitVector::operator&(BitVector const &other) const
{
  assert(size_ == other.size_);
  BitVector ret(*this);
  ret &= other;
  return ret;
}

inline void BitVector::InlineAndAssign(BitVector const &a, BitVector const &b)
{
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    assert(i < data_.size());

    data_[i] = a.data_[i] & b.data_[i];
  }
}

inline BitVector &BitVector::operator|=(BitVector const &other)
{
  assert(size_ == other.size_);
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    data_[i] |= other.data_[i];
  }

  return *this;
}

inline BitVector BitVector::operator|(BitVector const &other) const
{
  assert(size_ == other.size_);
  BitVector ret(*this);
  ret |= other;
  return ret;
}

inline void BitVector::conditional_flip(std::size_t block, std::size_t bit, uint64_t base)
{
  assert((base == 1) || (base == 0));
  data_[block] ^= base << bit;
}

inline void BitVector::conditional_flip(std::size_t bit, uint64_t base)
{
  conditional_flip(bit >> LOG_BITS, bit & BIT_MASK, base);
}

inline void BitVector::flip(std::size_t block, std::size_t bit)
{
  data_[block] ^= 1ull << bit;
}

inline void BitVector::flip(std::size_t bit)
{
  flip(bit >> LOG_BITS, bit & BIT_MASK);
}

inline BitVector::Block BitVector::bit(std::size_t block, std::size_t b) const
{
  assert(block < data_.size());
  return (data_[block] >> b) & 1u;
}

inline BitVector::Block BitVector::bit(std::size_t b) const
{
  return bit(b >> LOG_BITS, b & BIT_MASK);
}

inline void BitVector::set(std::size_t block, std::size_t bit, uint64_t val)
{
  uint64_t mask_bit = 1ull << bit;
  data_[block] &= ~mask_bit;
  data_[block] |= val << bit;
}

inline void BitVector::set(std::size_t bit, uint64_t val)
{
  set(bit >> LOG_BITS, bit & BIT_MASK, val);
}

inline BitVector::Block &BitVector::operator()(std::size_t n)
{
  return data_.At(n);
}

inline BitVector::Block const &BitVector::operator()(std::size_t n) const
{
  return data_.At(n);
}

inline std::size_t BitVector::size() const
{
  return size_;
}

inline uint32_t BitVector::log2_size() const
{
  return platform::ToLog2(static_cast<uint32_t>(size_));
}

inline std::size_t BitVector::blocks() const
{
  return blocks_;
}

inline BitVector::UnderlyingArray const &BitVector::data() const
{
  return data_;
}

inline BitVector::UnderlyingArray &BitVector::data()
{
  return data_;
}

inline std::size_t BitVector::PopCount() const
{
  std::size_t ret = 0;

  for (std::size_t i = 0; i < blocks_; ++i)
  {
    ret += static_cast<std::size_t>(__builtin_popcountl(data_[i]));
  }

  return std::min(ret, size_);
}

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


namespace serializers
{

template< typename D >
struct ArraySerializer< BitVector, D >
{
public:
  using Type       = BitVector;
  using DriverType = D;


  template< typename Constructor >
  static void Serialize(Constructor & array_constructor, Type const & mask)
  {
    uint64_t const bit_size   = mask.size();
    uint64_t const block_size = mask.blocks();    

    auto array = array_constructor(block_size + 1);

    auto const &underlying_blocks = mask.data();
    array.Append( bit_size );
    for (uint64_t i = 0; i < block_size; ++i)
    {
      array.Append(underlying_blocks[i]);
    }
  }

  template< typename ArrayDeserializer >
  static void Deserialize(ArrayDeserializer & array, Type & mask)
  {
    uint64_t bit_size   = 0;
    uint64_t block_size = array.size() - 1;

    array.GetNextValue(bit_size);

    mask.Resize(bit_size);
    assert(mask.blocks() == block_size);
    auto &underlying_blocks = mask.data();

    for(uint64_t i = 0; i < block_size; ++i)
    {
      array.GetNextValue(underlying_blocks[i]);
    }

  }  
};

};

/*
template <typename T>
void Serialize(T &s, BitVector const &mask)
{
  uint64_t const bit_size   = mask.size();
  uint64_t const block_size = mask.blocks();

  s << bit_size << block_size;

  auto const &underlying_blocks = mask.data();
  for (uint64_t i = 0; i < block_size; ++i)
  {
    s << underlying_blocks[i];
  }
}

template <typename T>
void Deserialize(T &s, BitVector &mask)
{
  uint64_t bit_size   = 0;
  uint64_t block_size = 0;

  s >> bit_size >> block_size;

  mask.Resize(bit_size);
  assert(mask.blocks() == block_size);

  auto &underlying_blocks = mask.data();
  for (uint64_t i = 0; i < block_size; ++i)
  {
    s >> underlying_blocks[i];
  }
}
*/

}  // namespace fetch
