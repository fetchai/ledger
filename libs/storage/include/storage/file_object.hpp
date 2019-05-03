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

//
//                      ┌───────────────┐       ┌────▶
//                      │               │       │
//                      │               ▼       │
//  ┌───────┬───────┬───────┬───────┬───────┬───────┐
//  │Obj1.1 │Obj1.2 │Obj2.1 │Obj1.3 │Obj2.2 │Obj3.1 │
//  │       │       │       │       │       │       │ .......
//  └───────┴───────┴───────┴───────┴───────┴───────┘
//      │       │               ▲
//      │       │               │
//      └───────┴───────────────┘

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/sha256.hpp"
#include "storage/cached_random_access_stack.hpp"
#include "storage/document.hpp"
#include "storage/storage_exception.hpp"
#include "storage/versioned_random_access_stack.hpp"
#include "vectorise/platform.hpp"

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

  uint64_t next = UNDEFINED;

  union
  {
    uint64_t previous = UNDEFINED;
    uint64_t free;
  };

  uint8_t data[BYTES];
};

/**
 * FileObject represents 'files' (objects) of varying length in a filesystem. Given the user knows
 * the starting index of their file, it can be recovered by traversing the stack.
 *
 * The file is stored on the stack as a doubly linked list, where each list element contains some
 * number of bytes of the file. This means that a high block size will increase wasted space and a
 * small block size will increase the file recovery time.
 *
 * In order to support deletion of blocks, the first block in the stack represents a doubly linked
 * list of all the free blocks. If there are free blocks this will be used when getting new blocks.
 *
 * The free block list is ordered, and files are ordered, to be incrementing in memory.
 *
 */
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

  FileObject()
    : block_number_(0)
    , byte_index_(HEADER_SIZE)
    , length_(HEADER_SIZE)
  {}

  ~FileObject()
  {
    Flush();
  }

  template <typename... Args>
  void Load(Args &&... args)
  {
    stack_.Load(std::forward<Args>(args)...);
    Initalise();
  }

  template <typename... Args>
  void New(Args &&... args)
  {
    stack_.New(std::forward<Args>(args)...);
    Initalise();
  }

  void Flush(bool /*lazy*/ = true)
  {
    block_type first;
    stack_.Get(id_, first);
    memcpy(first.data, reinterpret_cast<uint8_t const *>(&last_position_), sizeof(uint64_t));
    memcpy(first.data + sizeof(uint64_t), reinterpret_cast<uint8_t const *>(&length_),
           sizeof(uint64_t));
    stack_.Set(id_, first);

    stack_.Set(free_block_index_, free_block_);

    stack_.Flush();
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

  void Resize(uint64_t size)
  {
    Seek(0);

    length_            = HEADER_SIZE + size;
    uint64_t   last_bn = length_ / block_type::BYTES;
    block_type block;
    block_type prev_block;
    uint64_t   prev_block_index;

    // Expect these after a seek(0);
    assert(block_index_ == id_);
    assert(block_number_ == 0);

    // Traverse the doubly LL making sure each block number is valid.
    // First block must be valid.
    while (block_number_ < last_bn)
    {
      // Get the block at block_number
      if (block_index_ != block_type::UNDEFINED)
      {
        stack_.Get(block_index_, block);
      }
      else
      {
        if (block_number_ == 0)
        {
          throw StorageException("Resize failed: index invalid");
        }

        // Write new block
        block.previous = prev_block_index;
        assert(block.next == block_type::UNDEFINED);
        block_index_ = GetFreeBlock(block, prev_block_index);

        // Update prev block
        prev_block.next = block_index_;
        stack_.Set(prev_block_index, prev_block);
      }

      prev_block       = block;
      prev_block_index = block_index_;

      block_index_ = block.next;
      ++block_number_;
    }

    // Take care of extra blocks
    if (block.next != block_type::UNDEFINED)
    {
      FreeBlocks(block.next);
      block.next = block_type::UNDEFINED;
      stack_.Set(block_index_, block);
    }

    // Update state
    last_position_ = block_index_;
    block_count_   = block_number_;
    Seek(0);
  }

  void Write(byte_array::ConstByteArray const &arr)
  {
    Write(arr.pointer(), arr.size());
  }

  /**
   * Write bytes to file object, ensuring that the size of the file object is extended if
   * neccessary.
   *
   * @param: bytes The bytes to write
   * @param: num The number of bytes to write
   *
   */
  void Write(uint8_t const *bytes, uint64_t const &num)
  {
    uint64_t n = num + byte_index_ + block_number_ * block_type::BYTES;

    uint64_t last_block = platform::DivideCeil<uint64_t>(n, block_type::BYTES);

    --last_block;

    uint64_t first_bytes = block_type::BYTES - byte_index_;
    if (first_bytes > num)
    {
      first_bytes = num;
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
      block.next     = GetFreeBlock(empty, block_index_);
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
          block.next     = GetFreeBlock(empty, block_index_);
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

    uint64_t last_block = platform::DivideCeil<uint64_t>(n, block_type::BYTES);
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

  bool SeekFile(std::size_t const &position)
  {
    block_number_ = 0;
    byte_index_   = HEADER_SIZE;

    block_type first;
    assert(position < stack_.size());

    block_index_ = id_ = position;
    stack_.Get(id_, first);

    memcpy(reinterpret_cast<uint8_t *>(&last_position_), first.data, sizeof(uint64_t));
    memcpy(reinterpret_cast<uint8_t *>(&length_), first.data + sizeof(uint64_t), sizeof(uint64_t));

    block_count_ = platform::DivideCeil<uint64_t>(length_, block_type::BYTES);

    return true;
  }

  void CreateNewFile()
  {
    block_number_ = 0;
    byte_index_   = HEADER_SIZE;
    length_       = HEADER_SIZE;

    block_type block;
    last_position_ = stack_.size();

    memcpy(block.data, reinterpret_cast<uint8_t const *>(&last_position_), sizeof(uint64_t));
    memcpy(block.data + sizeof(uint64_t), reinterpret_cast<uint8_t const *>(&length_),
           sizeof(uint64_t));

    block_index_ = id_ = GetFreeBlock(block, 1);
    block_count_       = platform::DivideCeil<uint64_t>(length_, block_type::BYTES);
  }

  Document AsDocument()
  {
    Document ret;

    ret.document.Resize(this->size());
    this->Read(ret.document);

    return ret;
  }

  void Erase()
  {
    if (deletion_enabled)
    {
      FreeBlocks(block_index_);
    }
    Flush();
  }

  // TODO(HUT): figure out what to do about this abstraction breaking
  stack_type *stack()
  {
    return &stack_;
  }

  stack_type &underlying_stack()
  {
    return stack_;
  }

  bool deletion_enabled = false;

protected:
  stack_type stack_;
  uint64_t   id_;  // Location on stack of first block of file

  uint64_t block_number_;  // Nth block in file
  uint64_t block_count_;   // Number of blocks in file

  // Used while seeking within a file
  uint64_t block_index_;  // index of current block
  uint64_t byte_index_;   // index of current byte within block

  uint64_t length_ = 0;     // length in bytes of file. Should be at least HEADER_SIZE
  uint64_t last_position_;  // index of last block in file

  uint64_t   free_block_index_ = 0;
  block_type free_block_;

  uint64_t FreeBlocks()
  {
    return free_block_.free;
  }

private:
  /**
   * Initialise by looking for the block that's the beginning of our free blocks linked list. If
   * the stack is empty this means we set our own.
   */
  void Initalise()
  {
    if (stack_.size() == 0)
    {
      free_block_.free = 0;
      block_index_ = id_ = stack_.Push(free_block_);
    }
    else
    {
      stack_.Get(free_block_index_, free_block_);
    }
  }

  /**
   * Get free block from stack, write block to that location
   *
   * @param: block Block to write
   * @param: min_index Lower bound on index
   *
   * @return: Location on the stack of the free block
   */
  uint64_t GetFreeBlock(block_type const &write_block, uint64_t min_index)
  {
    // End of the stack is the default
    if (free_block_.free == 0)
    {
      return stack_.Push(write_block);
    }

    // Traverse the free block linked list to find first valid block
    block_type block;
    uint64_t   block_index = free_block_.next;
    assert(block_index != block_type::UNDEFINED);  // first block should be valid if free > 0

    do
    {
      // Reached end of free LL, default to end of stack
      if (block_index == block_type::UNDEFINED)
      {
        return stack_.Push(write_block);
      }

      stack_.Get(block_index, block);
    } while (block_index < min_index);

    // We now have the location of the point in the LL we want to remove
    // fix prev
    block_type mod_block;
    stack_.Get(block.previous, mod_block);
    mod_block.next = block.next;
    stack_.Set(block.previous, mod_block);

    // fix next
    if (block.next != block_type::UNDEFINED)
    {
      stack_.Get(block.next, mod_block);
      mod_block.previous = block.previous;
      stack_.Set(block.next, mod_block);
    }

    // Note there is no flush at this point
    assert(free_block_.free != 0);
    free_block_.free--;

    // Write and return
    stack_.Set(block_index, write_block);
    return block_index;
  }

  /**
   * Free blocks starting at index. We can't just append to the end of the free LL as we want to
   * maintain ordering.
   * Note that the block at remove_index must point to a valid LL.
   *
   * @param: remove_index Index of starting block to free
   */
  void FreeBlocks(uint64_t remove_index)
  {
    block_type remove_block;
    block_type prev_block;
    block_type next_block;
    uint64_t   free_index_p = free_block_index_;  // Starts at 0
    uint64_t   free_index_n = free_block_.next;

    // If no free blocks, safe to point the admin block to the beginning of this LL
    if (free_block_.free == 0)
    {
      assert(remove_index != block_type::UNDEFINED);
      free_block_.next = remove_index;

      // Count new free blocks
      while (remove_index != block_type::UNDEFINED)
      {
        stack_.Get(remove_index, remove_block);
        remove_index = remove_block.next;
        free_block_.free++;
      }
      return;
    }

    // We want to insert the blocks into the free blocks LL, do this by finding the block in the
    // free LL that we can insert before, so we have prev and next in the free LL, we then put the
    // remove_block in-between those.
    assert(free_index_n != block_type::UNDEFINED);
    stack_.Get(free_index_p, prev_block);    // <- pointing to free block LL-1
    stack_.Get(free_index_n, next_block);    // <- pointing to free block LL
    stack_.Get(remove_index, remove_block);  // Crawling the blocks to free

    while (remove_index != block_type::UNDEFINED)
    {
      // We want to insert when we're between prev and next or at the end
      if (free_index_n > remove_index || free_index_n == block_type::UNDEFINED)
      {
        // Save the next index
        uint64_t next_remove_index = remove_block.next;

        // Do the join and write back
        prev_block.next       = remove_index;
        next_block.previous   = remove_index;
        remove_block.previous = free_index_p;
        remove_block.next     = free_index_n;

        stack_.Set(free_index_p, prev_block);

        if (free_index_n != block_type::UNDEFINED)
        {
          stack_.Set(free_index_n, next_block);
        }

        stack_.Set(remove_index, remove_block);

        // Now we have inserted between prev and next, our removed block becomes prev. Free index
        // does not advance.
        prev_block   = remove_block;
        free_index_p = remove_index;

        remove_index = next_remove_index;
        stack_.Get(remove_index, remove_block);
        free_block_.free++;
      }
      else
      {
        prev_block   = next_block;
        free_index_p = free_index_n;
        free_index_n = next_block.next;

        stack_.Get(free_index_n, next_block);
      }
    }
  }
};
}  // namespace storage
}  // namespace fetch
