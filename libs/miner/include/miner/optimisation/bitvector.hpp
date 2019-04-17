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

#include <initializer_list>
#include <type_traits>
#include <vectorise/memory/shared_array.hpp>
#include <vectorise/platform.hpp>

namespace fetch {
namespace bitmanip {
namespace details {

class BitVectorImplementation
{
public:
  using data_type      = uint64_t;
  using container_type = memory::SharedArray<data_type>;

  enum
  {
    ELEMENT_BIT_SIZE = sizeof(data_type) << 3,
    LOG_BITS         = meta::Log2<8 * sizeof(data_type)>::value,
    BIT_MASK         = (1ull << LOG_BITS) - 1,
    SIMD_SIZE        = container_type::E_SIMD_COUNT
  };

  BitVectorImplementation()
  {
    Resize(0);
  }

  explicit BitVectorImplementation(std::size_t n)
  {
    Resize(n);
  }

  BitVectorImplementation(BitVectorImplementation const &other)
    : data_(other.data_.Copy())
    , size_(other.size_)
    , blocks_(other.blocks_)
  {}

  /**
   * Resize the vector to n bits
   *
   * @param bit_size The size in bits of the vector
   */
  void Resize(std::size_t bit_size)
  {
    container_type old = data_;

    // calculate
    std::size_t const num_elements = (bit_size + (ELEMENT_BIT_SIZE - 1)) / ELEMENT_BIT_SIZE;

    data_   = container_type(num_elements);
    blocks_ = num_elements;
    size_   = bit_size;

    if (bit_size > 0)
    {
      SetAllZero();  // TODO(issue 29): Only set those
    }

    // TODO(issue 29): Copy data;
  }

  void SetAllZero()
  {
    data_.SetAllZero();
  }

  void SetAllOne()
  {
    auto *buffer = reinterpret_cast<uint8_t *>(data_.pointer());
    std::memset(buffer, 0xFF, data_.size() * sizeof(data_type));
  }

  bool RemapTo(BitVectorImplementation &dst) const
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

  static bool Expand(BitVectorImplementation const &src, BitVectorImplementation &dst)
  {
    using BitVectorPtr = std::unique_ptr<BitVectorImplementation>;

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
    uint8_t const *src_buffer = reinterpret_cast<uint8_t const *>(src.data().pointer());
    uint16_t      *int_buffer = nullptr;
    uint16_t      *dst_buffer = reinterpret_cast<uint16_t *>(dst.data().pointer());

    // in cases larger than 1 and additional buffer is required
    if (num_loops > 1)
    {
      // create the intermediate vector
      intermediate_vector = std::make_unique<BitVectorImplementation>(dst.size());

      // update the intermediate buffer pointer
      int_buffer = reinterpret_cast<uint16_t *>(intermediate_vector->data().pointer());

      // in the case of even number of loops we need to swap the intermediate and destination buffers
      // to ensure the correct final destination
      if ((num_loops & 0x1) == 0)
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
        uint64_t const m = ((src_buffer[j] * 0x0101010101010101ULL) & 0x8040201008040201ULL) * 0x0102040810204081ULL;
        dst_buffer[j] = ((m >> 49u) & 0x5555) | ((m >> 48u) & 0xAAAA);
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

  static bool Contract(BitVectorImplementation const &src, BitVectorImplementation &dst)
  {
    using BitVectorPtr = std::unique_ptr<BitVectorImplementation>;

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
    uint16_t const *src_buffer = reinterpret_cast<uint16_t const *>(src.data().pointer());
    uint8_t        *int_buffer = nullptr;
    uint8_t        *dst_buffer = reinterpret_cast<uint8_t *>(dst.data().pointer());

    // in cases larger than 1 and additional buffer is required
    if (num_loops > 1)
    {
      // create the intermediate vector
      intermediate_vector = std::make_unique<BitVectorImplementation>(dst.size());

      // update the intermediate buffer pointer
      int_buffer = reinterpret_cast<uint8_t *>(intermediate_vector->data().pointer());

      // in the case of even number of loops we need to swap the intermediate and destination buffers
      // to ensure the correct final destination
      if ((num_loops & 0x1) == 0)
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
        uint16_t const a = (src_buffer[j] & 0x5555) | ((src_buffer[j] & 0xAAAA) >> 1u);
        dst_buffer[j] = static_cast<uint8_t>((a & 0x1u) | ((a >> 1u) & 0x2u) | ((a >> 2u) & 0x4u) | ((a >> 3u) & 0x8u) | ((a >> 4u) & 0x10u) | ((a >> 5u) & 0x20u) | ((a >> 6u) & 0x40u) | ((a >> 7u) & 0x80u));
      }

      // adjust the size in the case of multiple loops
      current_size_words /= 2;

      if (current_size_words == 0)
      {
        current_size_words= 1;
      }

      // the current destination buffer always becomes the next source buffer
      src_buffer = reinterpret_cast<uint16_t *>(dst_buffer);

      // swap the intermediate and destinations buffers ready for the next loop
      std::swap(int_buffer, dst_buffer);
    }

    return true;
  }

  bool operator==(BitVectorImplementation const &other) const
  {
    bool ret = this->size_ == other.size_;
    if (!ret)
    {
      return ret;
    }
    for (std::size_t i = 0; i < blocks_; ++i)
    {
      ret &= (this->operator()(i) == other(i));
    }
    return ret;
  }

  bool operator!=(BitVectorImplementation const &other)
  {
    return !(this->operator==(other));
  }

  BitVectorImplementation &operator^=(BitVectorImplementation const &other)
  {
    assert(size_ == other.size_);
    for (std::size_t i = 0; i < blocks_; ++i)
    {
      data_[i] ^= other.data_[i];
    }

    return *this;
  }

  BitVectorImplementation operator^(BitVectorImplementation const &other) const
  {
    assert(size_ == other.size_);
    BitVectorImplementation ret(*this);
    ret ^= other;
    return ret;
  }

  BitVectorImplementation &operator&=(BitVectorImplementation const &other)
  {
    assert(size_ == other.size_);
    for (std::size_t i = 0; i < blocks_; ++i)
    {
      data_[i] &= other.data_[i];
    }

    return *this;
  }

  BitVectorImplementation operator&(BitVectorImplementation const &other) const
  {
    assert(size_ == other.size_);
    BitVectorImplementation ret(*this);
    ret &= other;
    return ret;
  }

  void InlineAndAssign(BitVectorImplementation const &a, BitVectorImplementation const &b)
  {
    for (std::size_t i = 0; i < blocks_; ++i)
    {
      assert(i < data_.size());

      data_[i] = a.data_[i] & b.data_[i];
    }
  }

  BitVectorImplementation &operator|=(BitVectorImplementation const &other)
  {
    assert(size_ == other.size_);
    for (std::size_t i = 0; i < blocks_; ++i)
    {
      data_[i] |= other.data_[i];
    }

    return *this;
  }

  BitVectorImplementation operator|(BitVectorImplementation const &other) const
  {
    assert(size_ == other.size_);
    BitVectorImplementation ret(*this);
    ret |= other;
    return ret;
  }

  void conditional_flip(std::size_t const &block, std::size_t const &bit, uint64_t const &base)
  {
    assert((base == 1) || (base == 0));
    data_[block] ^= base << bit;
  }

  void conditional_flip(std::size_t const &bit, uint64_t const &base)
  {
    conditional_flip(bit >> LOG_BITS, bit & BIT_MASK, base);
  }

  void flip(std::size_t const &block, std::size_t const &bit)
  {
    data_[block] ^= 1ull << bit;
  }

  void flip(std::size_t const &bit)
  {
    flip(bit >> LOG_BITS, bit & BIT_MASK);
  }

  data_type bit(std::size_t const &block, std::size_t const &b) const
  {
    assert(block < data_.size());
    return (data_[block] >> b) & 1;
  }

  data_type bit(std::size_t const &b) const
  {
    return bit(b >> LOG_BITS, b & BIT_MASK);
  }

  void set(std::size_t const &block, std::size_t const &bit, uint64_t const &val)
  {
    uint64_t mask_bit = 1ull << bit;
    data_[block] &= ~mask_bit;
    data_[block] |= val << bit;
  }

  void set(std::size_t const &bit, uint64_t const &val)
  {
    set(bit >> LOG_BITS, bit & BIT_MASK, val);
  }

  data_type &operator()(std::size_t const &n)
  {
    return data_.At(n);
  }
  data_type const &operator()(std::size_t const &n) const
  {
    return data_.At(n);
  }

  std::size_t const &size() const
  {
    return size_;
  }
  std::size_t const &blocks() const
  {
    return blocks_;
  }
  container_type const &data() const
  {
    return data_;
  }
  container_type &data()
  {
    return data_;
  }

  std::size_t PopCount() const
  {
    std::size_t ret = 0;

    for (std::size_t i = 0; i < blocks_; ++i)
    {
      ret += static_cast<std::size_t>(__builtin_popcountl(data_[i]));
    }

    return ret;
  }

private:
  container_type data_;
  std::size_t    size_;
  std::size_t    blocks_;
};

inline std::ostream &operator<<(std::ostream &s, BitVectorImplementation const &b)
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

}  // namespace details

using BitVector = details::BitVectorImplementation;

}  // namespace bitmanip
}  // namespace fetch
