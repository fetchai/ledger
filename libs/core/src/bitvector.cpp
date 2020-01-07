//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/bitvector.hpp"
#include "vectorise/platform.hpp"

namespace fetch {

BitVector::BitVector(std::size_t n)
{
  Resize(n);
}

BitVector::BitVector(BitVector const &other)
  : data_(other.data_.Copy())
  , size_(other.size_)
  , blocks_(other.blocks_)
{}

/**
 * Resize the vector to n bits
 *
 * @param bit_size The size in bits of the vector
 */
void BitVector::Resize(std::size_t bit_size)
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

void BitVector::SetAllZero()
{
  data_.SetAllZero();
}

void BitVector::SetAllOne()
{
  auto *buffer = reinterpret_cast<uint8_t *>(data_.pointer());
  std::memset(buffer, 0xFF, data_.size() * sizeof(Block));
}

bool BitVector::RemapTo(BitVector &dst) const
{
  if (dst.size() == size())
  {
    dst = *this;
    return true;
  }
  if (dst.size() > size())
  {
    return Expand(*this, dst);
  }

  return Contract(*this, dst);
}

bool BitVector::Expand(BitVector const &src, BitVector &dst)
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

bool BitVector::Contract(BitVector const &src, BitVector &dst)
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

bool BitVector::operator==(BitVector const &other) const
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

bool BitVector::operator!=(BitVector const &other) const
{
  return !operator==(other);
}

BitVector &BitVector::operator^=(BitVector const &other)
{
  assert(size_ == other.size_);
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    data_[i] ^= other.data_[i];
  }

  return *this;
}

BitVector BitVector::operator^(BitVector const &other) const
{
  assert(size_ == other.size_);
  BitVector ret(*this);
  ret ^= other;
  return ret;
}

BitVector &BitVector::operator&=(BitVector const &other)
{
  assert(size_ == other.size_);
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    data_[i] &= other.data_[i];
  }

  return *this;
}

BitVector BitVector::operator&(BitVector const &other) const
{
  assert(size_ == other.size_);
  BitVector ret(*this);
  ret &= other;
  return ret;
}

void BitVector::InlineAndAssign(BitVector const &a, BitVector const &b)
{
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    assert(i < data_.size());

    data_[i] = a.data_[i] & b.data_[i];
  }
}

BitVector &BitVector::operator|=(BitVector const &other)
{
  assert(size_ == other.size_);
  for (std::size_t i = 0; i < blocks_; ++i)
  {
    data_[i] |= other.data_[i];
  }

  return *this;
}

BitVector BitVector::operator|(BitVector const &other) const
{
  assert(size_ == other.size_);
  BitVector ret(*this);
  ret |= other;
  return ret;
}

void BitVector::conditional_flip(std::size_t block, std::size_t bit, uint64_t base)
{
  assert((base == 1) || (base == 0));
  data_[block] ^= base << bit;
}

void BitVector::conditional_flip(std::size_t bit, uint64_t base)
{
  conditional_flip(bit >> LOG_BITS, bit & BIT_MASK, base);
}

void BitVector::flip(std::size_t block, std::size_t bit)
{
  data_[block] ^= 1ull << bit;
}

void BitVector::flip(std::size_t bit)
{
  flip(bit >> LOG_BITS, bit & BIT_MASK);
}

BitVector::Block BitVector::bit(std::size_t block, std::size_t b) const
{
  assert(block < data_.size());
  return (data_[block] >> b) & 1u;
}

BitVector::Block BitVector::bit(std::size_t b) const
{
  return bit(b >> LOG_BITS, b & BIT_MASK);
}

void BitVector::set(std::size_t block, std::size_t bit, uint64_t val)
{
  uint64_t mask_bit = 1ull << bit;
  data_[block] &= ~mask_bit;
  data_[block] |= val << bit;
}

void BitVector::set(std::size_t bit, uint64_t val)
{
  set(bit >> LOG_BITS, bit & BIT_MASK, val);
}

BitVector::Block &BitVector::operator()(std::size_t n)
{
  return data_.At(n);
}

BitVector::Block const &BitVector::operator()(std::size_t n) const
{
  return data_.At(n);
}

std::size_t BitVector::size() const
{
  return size_;
}

uint32_t BitVector::log2_size() const
{
  return platform::ToLog2(static_cast<uint32_t>(size_));
}

std::size_t BitVector::blocks() const
{
  return blocks_;
}

BitVector::UnderlyingArray const &BitVector::data() const
{
  return data_;
}

BitVector::UnderlyingArray &BitVector::data()
{
  return data_;
}

std::size_t BitVector::PopCount() const
{
  std::size_t ret = 0;

  for (std::size_t i = 0; i < blocks_; ++i)
  {
    ret += static_cast<std::size_t>(platform::CountSetBits(data_[i]));
  }

  return std::min(ret, size_);
}

std::ostream &operator<<(std::ostream &s, BitVector const &b)
{
#if 1
  for (std::size_t i = 0; i < b.size(); ++i)
  {
    if ((i != 0u) && ((i % 10) == 0))
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

BitVector::Iterator BitVector::begin() const
{
  return Iterator{*this, true};
}

BitVector::Iterator BitVector::end() const
{
  return Iterator{*this};
}

BitVector::Iterator::Iterator(BitVector const &container, bool const is_begin)
  : container_{container}
  , end_{container.size()}
  , index_{end_}
{
  if (is_begin)
  {
    Next(is_begin);
  }
}

bool BitVector::Iterator::operator==(Iterator const &right) const
{
  return index_ == right.index_ && &container_ == &right.container_;
}

bool BitVector::Iterator::operator!=(Iterator const &right) const
{
  return !(*this == right);
}

BitVector::Iterator::value_type BitVector::Iterator::operator*() const
{
  return index_;
}

BitVector::Iterator &BitVector::Iterator::operator++()
{
  return Next();
}

BitVector::Iterator &BitVector::Iterator::Next(bool is_begin)
{
  if (!is_begin && index_ >= end_)
  {
    return *this;
  }

  auto const &data{container_.data()};
  auto const  blocks_count{container_.blocks()};

  BitVector::Block shift_mask{~1ull};

  if (is_begin)
  {
    index_     = 0;
    shift_mask = ~0ull;
  }

  for (std::size_t blck_idx{index_ >> BitVector::LOG_BITS}; blck_idx < blocks_count; ++blck_idx)
  {
    std::size_t            bit_idx{index_ & BitVector::BIT_MASK};
    BitVector::Block       mask{(shift_mask << bit_idx)};
    BitVector::Block const x{data[blck_idx] & mask};
    std::size_t const      trailing_zeroes{platform::CountTrailingZeroes64(x)};
    index_ += (trailing_zeroes - bit_idx);

    if (BitVector::ELEMENT_BIT_SIZE > trailing_zeroes)
    {
      break;
    }

    shift_mask = ~0ull;
  }

  return *this;
}

}  // namespace fetch
