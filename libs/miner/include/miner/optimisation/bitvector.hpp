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
    LOG_BITS         = meta::Log2(8 * sizeof(data_type)),
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

  bool operator==(BitVectorImplementation const &other) const
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

  bool operator!=(BitVectorImplementation const &other) const
  {
    return !operator==(other);
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
