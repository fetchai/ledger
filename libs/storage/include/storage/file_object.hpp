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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/sha256.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/storage_exception.hpp"
#include "storage/versioned_random_access_stack.hpp"

#include <cstdint>

namespace fetch {
namespace storage {

template <std::size_t BS = 2>
struct FileBlockType
{
  enum
  {
    BYTES     = BS + 2 * sizeof(uint64_t),
    UNDEFINED = uint64_t(-1)
  };

  FileBlockType()
  {
    // Ensures that padded bytes are not uninitialized.
    memset(this, 0, sizeof(decltype(*this)));
    previous = UNDEFINED;
    next     = UNDEFINED;
  }

  FileBlockType(uint64_t const &prev, uint8_t const *bytes, std::size_t const &n = std::size_t(-1))
  {
    memset(this, 0, sizeof(decltype(*this)));
    memcpy(data, bytes, std::min(n, std::size_t(BYTES)));
    previous = prev;
    next     = UNDEFINED;
  }

  uint64_t previous = UNDEFINED;
  uint64_t next     = UNDEFINED;
  uint8_t  data[BYTES];
};

template <typename S = VersionedRandomAccessStack<FileBlockType<>>>
class FileObject
{
public:
  // TODO(issue 10): make pluggable
  using stack_type  = S;
  using block_type  = typename stack_type::type;
  using hasher_type = crypto::SHA256;

  static constexpr char const *LOGGING_NAME = "FileObject";

  enum
  {
    HEADER_SIZE = 2 * sizeof(uint64_t)
  };

  FileObject(FileObject const &other) = delete;
  FileObject operator=(FileObject const &other) = delete;
  FileObject(FileObject &&other)                = default;
  FileObject &operator=(FileObject &&other) = default;

  FileObject(stack_type &stack)
    : stack_(stack)
    , block_number_(0)
    , byte_index_(HEADER_SIZE)
    , length_(HEADER_SIZE)
  {
    block_type block;
    last_position_ = stack_.size();

    memcpy(block.data, reinterpret_cast<uint8_t const *>(&last_position_), sizeof(uint64_t));
    memcpy(block.data + sizeof(uint64_t), reinterpret_cast<uint8_t const *>(&length_),
           sizeof(uint64_t));

    block_index_ = id_ = stack_.Push(block);

    block_count_ = length_ / block_type::BYTES;
    if (block_count_ * block_type::BYTES < length_)
    {
      ++block_count_;
    }
  }

  FileObject(stack_type &stack, std::size_t const &position)
    : stack_(stack)
    , block_number_(0)
    , byte_index_(HEADER_SIZE)
  {
    block_type first;
    assert(position < stack_.size());

    block_index_ = id_ = position;
    stack_.Get(id_, first);

    memcpy(reinterpret_cast<uint8_t *>(&last_position_), first.data, sizeof(uint64_t));
    memcpy(reinterpret_cast<uint8_t *>(&length_), first.data + sizeof(uint64_t), sizeof(uint64_t));

    // Previously written blocks should at least have header size - if not something has gone wrong
    assert(length_ >= HEADER_SIZE);

    block_count_ = length_ / block_type::BYTES;
    if (block_count_ * block_type::BYTES < length_)
    {
      ++block_count_;
    }
  }

  ~FileObject()
  {
    Flush();
  }

  void Flush()
  {
    block_type first;
    stack_.Get(id_, first);
    memcpy(first.data, reinterpret_cast<uint8_t const *>(&last_position_), sizeof(uint64_t));
    memcpy(first.data + sizeof(uint64_t), reinterpret_cast<uint8_t const *>(&length_),
           sizeof(uint64_t));
    stack_.Set(id_, first);
    // TODO(issue 10): Flush    stack_.Flush();
  }

  void Seek(uint64_t n)
  {
    n += HEADER_SIZE;
    uint64_t   next_bn = n / block_type::BYTES;
    block_type block;

    while (block_number_ < next_bn)
    {
      stack_.Get(block_index_, block);
      block_index_ = block.next;
      ++block_number_;
      assert(block_index_ != block_type::UNDEFINED);

      if (block_number_ >= block_count_)
      {
        throw StorageException("Seek is out of bounds");
      }
    }

    while (block_number_ > next_bn)
    {
      stack_.Get(block_index_, block);
      block_index_ = block.previous;
      --block_number_;
      assert(block_index_ != block_type::UNDEFINED);

      if (block_number_ >= block_count_)
      {
        throw StorageException("Seek is out of bounds");
      }
    }

    byte_index_ = n % block_type::BYTES;
    assert((block_number_ != 0) || (block_index_ == id_));
  }

  uint64_t Tell()
  {
    if ((block_index_ == 0) && (byte_index_ < HEADER_SIZE))
    {
      return 0;
    }
    return block_index_ * block_type::BYTES + byte_index_ - HEADER_SIZE;
  }

  void Shrink(uint64_t const &size)
  {
    assert(length_ > (HEADER_SIZE + size));

    Seek(0);

    length_            = HEADER_SIZE + size;
    uint64_t   last_bn = length_ / block_type::BYTES;
    block_type block;

    while (block_number_ < last_bn)
    {
      stack_.Get(block_index_, block);
      block_index_ = block.next;
      ++block_number_;
      assert(block_index_ != block_type::UNDEFINED);

      if (block_number_ >= block_count_)
      {
        throw StorageException("Seek is out of bounds");
      }
    }

    last_position_ = block_index_;
    block_count_   = block_number_;

    // TODO(issue 10): Delete whatever comes after
  }

  void Grow(uint64_t size)
  {
    Seek(0);
    size += HEADER_SIZE;
    throw StorageException("Grow is not implemented yet");
  }

  void Write(byte_array::ConstByteArray const &arr)
  {
    Write(arr.pointer(), arr.size());
  }

  void Write(uint8_t const *bytes, uint64_t const &m)
  {
    uint64_t n = m + byte_index_ + block_number_ * block_type::BYTES;

    uint64_t last_block = (n) / block_type::BYTES;
    if (last_block * block_type::BYTES < n)
    {
      ++last_block;
    }
    --last_block;

    uint64_t first_bytes = block_type::BYTES - byte_index_;
    if (first_bytes > m)
    {
      first_bytes = m;
    }

    uint64_t last_bytes = n - (block_type::BYTES * last_block);

    // Writing first
    block_type block;
    stack_.Get(block_index_, block);

    if (((last_block != 0) && (block.next == block_type::UNDEFINED)) ||
        ((last_block == 0) && (((byte_index_ + first_bytes) % block_type::BYTES) == 0)))
    {
      block_type empty;
      empty.previous = block_index_;
      block.next     = stack_.Push(empty);
    }

    memcpy(block.data + byte_index_, bytes, first_bytes);
    stack_.Set(block_index_, block);
    byte_index_ = (byte_index_ + first_bytes) % block_type::BYTES;

    if (last_block == 0)
    {
      if (byte_index_ == 0)
      {
        block_index_ = block.next;
        ++block_number_;
      }
    }
    else
    {
      uint64_t offset = first_bytes;

      // Middle last_block
      --last_block;
      while (block_number_ < last_block)
      {
        assert(byte_index_ == 0);
        ++block_number_;
        block_index_ = block.next;

        stack_.Get(block_index_, block);

        if (block.next == block_type::UNDEFINED)
        {
          block_type empty;
          empty.previous = block_index_;
          block.next     = stack_.Push(empty);
        }

        memcpy(block.data, bytes + offset, block_type::BYTES);
        stack_.Set(block_index_, block);

        offset += block_type::BYTES;
      }

      // Last block
      if (block_number_ == last_block)
      {
        ++block_number_;
        block_index_ = block.next;
        stack_.Get(block_index_, block);
        memcpy(block.data, bytes + offset, last_bytes);
        stack_.Set(block_index_, block);
        byte_index_ = last_bytes;
      }
    }

    uint64_t position = byte_index_ + block_number_ * block_type::BYTES;
    if (position > length_)
    {
      length_ = position;
    }

    if (block_number_ > block_count_)
    {
      last_position_ = block_index_;
      block_count_   = block_number_;
    }

    Flush();
  }

  void Read(byte_array::ByteArray &arr)
  {
    Read(arr.pointer(), arr.size());
  }

  void Read(uint8_t *bytes, uint64_t const &m)
  {
    uint64_t n = m + byte_index_ + block_number_ * block_type::BYTES;

    uint64_t last_block = (n) / block_type::BYTES;
    if (last_block * block_type::BYTES < n)
    {
      ++last_block;
    }
    --last_block;

    uint64_t first_bytes = block_type::BYTES - byte_index_;
    if (first_bytes > m)
    {
      first_bytes = m;
    }

    uint64_t last_bytes = n - (block_type::BYTES * last_block);

    // Writing first
    assert(block_index_ < stack_.size());

    block_type block;
    stack_.Get(block_index_, block);

    if ((last_block != 0) && (block.next == block_type::UNDEFINED))
    {
      throw StorageException("Could not read block");
    }

    memcpy(bytes, block.data + byte_index_, first_bytes);
    byte_index_ = (byte_index_ + first_bytes) % block_type::BYTES;

    if (last_block != 0)
    {
      uint64_t offset = first_bytes;

      // Middle last block
      --last_block;
      while (block_number_ < last_block)
      {
        assert(byte_index_ == 0);
        ++block_number_;
        block_index_ = block.next;

        stack_.Get(block_index_, block);

        if (block.next == block_type::UNDEFINED)
        {
          throw StorageException("Could not read block");
        }

        memcpy(bytes + offset, block.data, block_type::BYTES);

        offset += block_type::BYTES;
      }

      // Last block
      if (block_number_ == last_block)
      {
        ++block_number_;
        block_index_ = block.next;
        stack_.Get(block_index_, block);
        memcpy(bytes + offset, block.data, last_bytes);

        byte_index_ = last_bytes;
      }
    }
  }

  uint64_t const &id() const
  {
    return id_;
  }

  uint64_t size() const
  {
    return length_ - HEADER_SIZE;
  }

  byte_array::ConstByteArray Hash()
  {
    hasher_type hasher;
    hasher.Reset();
    UpdateHash(hasher);
    return hasher.Final();
  }

  void UpdateHash(crypto::StreamHasher &hasher)
  {
    uint64_t bi        = id_;
    uint64_t remaining = length_ - HEADER_SIZE;

    block_type block;

    stack_.Get(bi, block);
    uint64_t n = std::min(remaining, uint64_t(block_type::BYTES - HEADER_SIZE));
    hasher.Update(block.data + HEADER_SIZE, n);

    remaining -= n;
    while (remaining != 0)
    {

      bi = block.next;
      if (bi == block_type::UNDEFINED)
      {
        throw StorageException("File corrupted");
      }

      stack_.Get(bi, block);
      n = std::min(remaining, uint64_t(block_type::BYTES));
      hasher.Update(block.data, n);

      remaining -= n;
    }
  }

private:
  uint64_t    id_;
  stack_type &stack_;

  uint64_t block_number_;
  uint64_t block_count_;

  uint64_t block_index_;
  uint64_t byte_index_;
  uint64_t length_ = 0, last_position_;
};
}  // namespace storage
}  // namespace fetch
