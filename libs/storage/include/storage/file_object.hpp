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

template <std::size_t BS = 128>
struct FileBlockType
{
  static constexpr uint64_t META_DATA_BYTES = (2 * sizeof(uint64_t));
  static constexpr uint64_t CAPACITY        = BS - META_DATA_BYTES;
  static constexpr uint64_t UNDEFINED       = std::numeric_limits<uint64_t>::max();

  static_assert(BS > META_DATA_BYTES, "Block size needs to exceed the min requirement for metadata");

  FileBlockType()
  {
    // Ensures that padded bytes are not uninitialized.
    memset(this, 0, sizeof(decltype(*this)));
    previous         = UNDEFINED;
    next             = UNDEFINED;
    file_object_size = UNDEFINED;
  }

  // Metadata
  uint64_t next     = UNDEFINED;
  uint64_t previous = UNDEFINED; // For the free block, this points to the end of the LL.

  // Either the size of the file object, or in the case this block is
  // used as the 'free linked list', this is the number of free blocks.
  // this is only valid for the first block.
  union
  {
    uint64_t file_object_size = UNDEFINED;
    uint64_t free_blocks;
  };

  // Data
  uint8_t data[CAPACITY];
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
 * The free block list is ordered, and files are ordered, to be incrementing in memory. This is
 * in order to reduce programmer error.
 *
 */
template <typename S = VersionedRandomAccessStack<FileBlockType<>>>
class FileObject
{
public:
  using stack_type  = S;
  using block_type  = typename stack_type::type;
  using hasher_type = crypto::SHA256;

  static constexpr char const *LOGGING_NAME = "FileObject";

  enum
  {
    HEADER_SIZE = 2 * sizeof(uint64_t)
  };

  FileObject(FileObject const &other)           = delete;
  FileObject operator=(FileObject const &other) = delete;
  FileObject(FileObject &&other)                = default;
  FileObject &operator=(FileObject &&other)     = default;

  FileObject();
  virtual ~FileObject();

  // TODO(HUT): unify this new/load methodology in constructor (?) etc.
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

  void Flush(bool /*lazy*/ = true);

  void Seek(uint64_t index);

  uint64_t Tell();

  void Resize(uint64_t size);

  void Write(byte_array::ConstByteArray const &arr);

  void Write(uint8_t const *bytes, uint64_t const &num);

  void Read(byte_array::ByteArray &arr);

  void Read(uint8_t *bytes, uint64_t const &m);

  uint64_t const &id() const;

  uint64_t FileObjectSize() const;

  // TODO(HUT): look into/consider deleting
  byte_array::ConstByteArray Hash();

  // TODO(HUT): move to private?
  void UpdateHash(crypto::StreamHasher &hasher);

  bool SeekFile(std::size_t const &position);

  void CreateNewFile(uint64_t size = 0);

  Document AsDocument();

  void Erase();

  bool VerifyConsistency(std::vector<uint64_t> const &ids);

  // TODO(HUT): figure out what to do about this abstraction breaking
  stack_type *stack();
  stack_type &underlying_stack();

  // TODO(HUT): make private (?)
protected:
  stack_type stack_;

  // tracking variables relating to current file object
  uint64_t id_                = 0;  // Location on stack of first block of file
  uint64_t block_number_      = 0;  // Nth block in file we are looking at
  uint64_t block_index_       = 0;  // index of block_number_ on the stack
  uint64_t byte_index_        = 0;  // index of current byte within BLOCK
  uint64_t byte_index_global_ = 0;  // index of current byte within file // TODO(HUT): del?
  uint64_t length_            = 0;  // length in bytes of file. // TODO(HUT): make sure this is correct - also can be found from Get(id) right - any point in keeping?

  // TODO(HUT): block_type -> BlockType etc.
  static constexpr uint64_t   free_block_index_ = 0; // Location of the meta 'free block'

private:
  void Initalise();

  enum class Action
  {
    READ,
    WRITE
  };

  uint64_t GetFreeBlocks(uint64_t min_index, uint64_t num);

  uint64_t FreeBlocks();

  void FreeBlocksInList(uint64_t remove_index);

  void ReadWriteHelper(uint8_t const *bytes, uint64_t const &num, Action action);
};

// TODO(HUT): done
template <typename S>
FileObject<S>::FileObject() = default;

// TODO(HUT): done
template <typename S>
FileObject<S>::~FileObject()
{
  // TODO(HUT): don't flush in destructors, except the highest level storage code (?)
}

// TODO(HUT): done
template <typename S>
void FileObject<S>::Flush(bool lazy)
{
  stack_.Flush(lazy);
}

// TODO(HUT): done
template <typename S>
void FileObject<S>::Seek(uint64_t index)
{
  byte_index_global_     = index;
  byte_index_            = index % block_type::CAPACITY;
  uint64_t desired_bn    = index / block_type::CAPACITY;
  block_type block;

  // Optionally start seeking from the first block if that's faster
  if(desired_bn < block_number_ && block_number_ - desired_bn < desired_bn)
  {
    block_number_ = 0;
  }

  // need to find index of this block on stack
  while(block_number_ != desired_bn)
  {
    if(block_number_ == block_type::UNDEFINED)
    {
      throw StorageException("Attempt to seek to an invalid block");
    }

    stack_.Get(block_number_, block);

    if(desired_bn < block_number_)
    {
      block_number_ = block.previous;
    }
    else
    {
      block_number_ = block.next;
    }
  }
}

// TODO(HUT): done
// TODO(HUT): const things
template <typename S>
uint64_t FileObject<S>::Tell()
{
  return byte_index_global_;
}

template <typename S>
void FileObject<S>::Resize(uint64_t size)
{
  // Reset variables which might otherwise point to invalid locations
  Seek(0);
  block_type block;

  // Update block 0 with new size
  {
    stack_.Get(id_, block);

    block.file_object_size = size;
    stack_.Set(id_, block);
    length_ = size;
  }

  // Traverse the blocks until the target blocks criteria is fulfilled.
  uint64_t target_blocks = platform::DivideCeil<uint64_t>(size, block_type::CAPACITY);

  // corner case when size is 0 - we need at least one block per file
  target_blocks = target_blocks == 0 ? 1 : target_blocks;

  uint64_t block_number = 1;
  uint64_t block_index  = id_;

  for(;;)
  {
    stack_.Get(block_index, block);

    uint64_t block_index_tmp = block_index;
    block_index = block.next;

    // if we reach the end of the LL, append as many blocks as we need
    if(block.next == block_type::UNDEFINED && block_number != target_blocks)
    {
      block.next = GetFreeBlocks(block_index_tmp, target_blocks - block_number);
      stack_.Set(block_index_tmp, block);

      uint64_t next_tmp = block.next;

      // need to update the new blocks' backward references too
      stack_.Get(next_tmp, block);
      block.previous = block_index_tmp;
      stack_.Set(next_tmp, block);
      break;
    }

    // If we reach as many blocks we actually need, free the rest
    if(block_number == target_blocks)
    {
      FreeBlocksInList(block_index);

      // Update last block to terminate correctly
      stack_.Get(block_index_tmp, block);
      block.next = block_type::UNDEFINED;
      stack_.Set(block_index_tmp, block);
      break;
    }

    block_number++;
  }
}

// TODO(HUT): done
template <typename S>
void FileObject<S>::Write(byte_array::ConstByteArray const &arr)
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
template <typename S>
void FileObject<S>::ReadWriteHelper(uint8_t const *bytes, uint64_t const &num, Action action)
{
  assert(id_ != 0 && "Attempt to write to the free block as if it were a file is a programmer error in FileObject");

  uint64_t   bytes_left_to_write       = num;
  uint64_t   bytes_to_write_in_block   = std::min(block_type::CAPACITY - byte_index_, bytes_left_to_write);
  block_type block_being_written;
  uint64_t   block_index_being_written = block_index_;
  uint64_t   byte_index                = byte_index_;
  uint64_t   bytes_offset              = 0;

  assert(bytes_to_write_in_block <= block_type::CAPACITY);

  while(bytes_left_to_write > 0)
  {
    // TODO(HUT): switch to explicit ref here
    assert(block_index_being_written != uint64_t(-1) && "Invalid block index during writing");
    // Get block, write data to it
    stack_.Get(block_index_being_written, block_being_written);

    // TODO(HUT): 'write' -> copy
    switch (action)
    {
      case Action::READ:
        memcpy((uint8_t *)(bytes + bytes_offset), block_being_written.data + byte_index,  bytes_to_write_in_block);
      break;
      case Action::WRITE:
        memcpy(block_being_written.data + byte_index, bytes + bytes_offset, bytes_to_write_in_block);
      break;
    }

    // Write block back
    byte_index                = 0;
    stack_.Set(block_index_being_written, block_being_written);
    block_index_being_written = block_being_written.next;

    bytes_offset        += bytes_to_write_in_block;
    bytes_left_to_write -= bytes_to_write_in_block;
    bytes_to_write_in_block = std::min(block_type::CAPACITY - byte_index_, bytes_left_to_write);
  }
}

// TODO(HUT): done
// TODO(HUT): num -> bytes_left_to_write
template <typename S>
void FileObject<S>::Write(uint8_t const *bytes, uint64_t const &num)
{
  ReadWriteHelper(bytes, num, Action::WRITE);
}

// TODO(HUT): done
template <typename S>
void FileObject<S>::Read(byte_array::ByteArray &arr)
{
  Read(arr.pointer(), arr.size());
}

// TODO(HUT): rename m -> num
template <typename S>
void FileObject<S>::Read(uint8_t *bytes, uint64_t const &m)
{
  ReadWriteHelper(bytes, m, Action::READ);
}

// TODO(HUT): done
template <typename S>
uint64_t const &FileObject<S>::id() const
{
  return id_;
}

template <typename S>
uint64_t FileObject<S>::FileObjectSize() const
{
  return length_;
}

// TODO(HUT): done
// TODO(HUT): remove?
template <typename S>
byte_array::ConstByteArray FileObject<S>::Hash()
{
  hasher_type hasher;
  hasher.Reset();
  UpdateHash(hasher);
  return hasher.Final();
}

// TODO(HUT): done
template <typename S>
void FileObject<S>::UpdateHash(crypto::StreamHasher &hasher)
{
  Seek(0);
  byte_array::ByteArray arr;
  arr.Resize(length_);
  Read(arr);
  hasher.Update(arr.pointer(), length_);
}

// TODO(HUT): done
template <typename S>
bool FileObject<S>::SeekFile(std::size_t const &position)
{
  if(position >= stack_.size())
  {
    throw StorageException("Attempt to seek file past stack end");
  }

  block_index_ = id_ = position;
  block_number_      = 0;
  byte_index_        = 0;
  byte_index_global_ = 0;

  // Need to retrieve the block to determine length
  block_type block;
  stack_.Get(id_, block);

  length_ = block.file_object_size;

  return true;
}

// TODO(HUT): done
template <typename S>
void FileObject<S>::CreateNewFile(uint64_t size)
{
  block_number_          = 0;
  byte_index_            = 0;
  byte_index_global_     = 0;
  length_                = size;
  uint64_t target_blocks = platform::DivideCeil<uint64_t>(size, block_type::CAPACITY);

  // corner case when size is 0 - we need at least one block per file
  target_blocks = target_blocks == 0 ? 1 : target_blocks;

  block_index_ = id_ = GetFreeBlocks(1, target_blocks);
}

// TODO(HUT): done
template <typename S>
Document FileObject<S>::AsDocument()
{
  Document ret;

  if(byte_index_global_ > length_)
  {
    throw StorageException("global byte index exceeds length when attempting to retrieve file object");
  }

  ret.document.Resize(length_ - byte_index_global_);
  this->Read(ret.document);

  return ret;
}

// TODO(HUT): done
template <typename S>
void FileObject<S>::Erase()
{
  FreeBlocksInList(id_);
  id_     = std::numeric_limits<uint64_t>::max();
  length_ = 0;
}

// TODO(HUT): figure out what to do about this abstraction breaking
template <typename S>
S *FileObject<S>::stack()
{
  return &stack_;
}

template <typename S>
S &FileObject<S>::underlying_stack()
{
  return stack_;
}

// TODO(HUT): rename this overloading
/**
 * Get the number of free blocks that are available in the stack without
 * having to increase its size
 *
 * @return: Number of free blocks
 */
template <typename S>
uint64_t FileObject<S>::FreeBlocks()
{
  block_type free_block;
  stack_.Get(free_block_index_, free_block);

  return free_block.free_blocks;
}

/**
 * Initialise by looking for the block that's the beginning of our free blocks linked list. If
 * the stack is empty this means we set our own.
 */
// TODO(HUT): done
template <typename S>
void FileObject<S>::Initalise()
{
  block_type free_block;
  free_block.free_blocks = 0;
  free_block.next        = 0;
  free_block.previous    = 0;
  block_index_           = 0;
  id_                    = 0;

  if (stack_.size() == 0)
  {
    stack_.Push(free_block);
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
template <typename S>
uint64_t FileObject<S>::GetFreeBlocks(uint64_t min_index, uint64_t num)
{
  if(num == 0)
  {
    throw StorageException("Attempt to get 0 free blocks is invalid");
  }

  auto DefaultFreeAllocation = [this](uint64_t num)
  {
    if(num == 0)
    {
      throw StorageException("Attempt to allocate 0 free blocks is invalid");
    }

    block_type block;
    uint64_t first_block_index = stack_.size();

    for (uint64_t i = 0; i < num; ++i)
    {
      block.previous = (i == 0)     ? block_type::UNDEFINED : first_block_index + i - 1;
      block.next     = (i == num-1) ? block_type::UNDEFINED : first_block_index + i + 1;
      stack_.Push(block);
    }

    return first_block_index;
  };

  block_type free_block;
  block_type block;
  stack_.Get(free_block_index_, free_block);

  // Only allow free blocks to be taken from the free LL in the scenario that there are enough
  // blocks in the LL above min_index. This way it is easier to traverse from the end of the stack
  // backwards, then 'cut' the LL at that point. Check here for easy exit.
  if (free_block.free_blocks == 0 || free_block.previous < min_index || free_block.free_blocks < num)
  {
    return DefaultFreeAllocation(num);
  }

  uint64_t index             = free_block.previous;
  uint64_t free_blocks_in_ll = 1;

  // Traverse backwards in the stack
  for(;;)
  {
    stack_.Get(index, block);
    index = block.previous;
    free_blocks_in_ll++;

    if(index < min_index)
    {
      break;
    }

    if(index == free_block_index_)
    {
      break;
    }

    // This case has found it successfully
    if(free_blocks_in_ll == num)
    {
      // Point end of LL back to free block
      uint64_t index_prev = block.previous;
      stack_.Get(index_prev, block);
      block.next = free_block_index_;
      stack_.Set(index_prev, block);

      // Update free block
      assert(free_block.free_blocks >= num);
      free_block.free_blocks -= num;
      free_block.previous = index;
      stack_.Set(free_block_index_, free_block);

      return index;
    }
  }

  // Failed to cut the stack.
  return DefaultFreeAllocation(num);
}

/**
 * Free blocks starting at index. We can't just append to the end of the free LL as we want to
 * maintain ordering.
 * Note that the block at remove_index must point to a valid LL, the last block of which points to 'invalid'
 *
 *
 * @param: remove_index Index of starting block to free
 */
template <typename S>
void FileObject<S>::FreeBlocksInList(uint64_t remove_index)
{
  // Maintain two 'pointers' to the first free block and the next. Advance these two pointers,
  // considering remove_index for insertion between them at each step. If we reach the end of
  // the LL, append all of the remove blocks to the free LL.
  block_type remove_block;
  block_type free_block;
  block_type free_block_next;

  stack_.Get(free_block_index_, free_block);

  uint64_t free_block_index      = free_block_index_;
  uint64_t free_block_next_index = free_block.next;
  uint64_t free_blocks_added     = 0;

  bool insert_mode = true;

  while(remove_index != block_type::UNDEFINED)
  {
    stack_.Get(free_block_index,      free_block);
    stack_.Get(free_block_next_index, free_block_next);
    stack_.Get(remove_index,          remove_block);

    if(insert_mode)
    {
      // Reached the end of the LL and wrapped around
      if(free_block_next_index == 0)
      {
        insert_mode = false;
        continue;
      }

      // Insert the remove block iff it sits between the pointers
      if(remove_index > free_block_index && remove_index < free_block_next_index)
      {
        uint64_t next_remove_index = remove_block.next;

        // Zero the removed block
        remove_block = block_type{};

        // Adjust remove block references
        remove_block.previous = free_block_index;
        remove_block.next     = free_block_next_index;
        stack_.Set(remove_index, remove_block);

        // Adjust 'prev' block to this
        free_block.next = remove_index;
        stack_.Set(free_block_index, free_block);

        // Adjust 'next' block to this
        free_block_next.previous = remove_index;
        stack_.Set(free_block_next_index, free_block_next);

        // Update - removed block is now next
        free_block_next_index = remove_index;
        remove_index          = next_remove_index;
        free_blocks_added++;
      }
      else
      {
        free_block_index      = free_block_next_index;
        free_block_next_index = free_block_next.next;
      }
    }
    else
    {
      // In this case we are appending to the end of our free blocks LL. free_block should
      // always be the last block in the LL.
      free_block.next = remove_index;
      stack_.Set(free_block_index, free_block);

      uint64_t next_remove_index = remove_block.next;

      remove_block          = block_type{};
      remove_block.previous = free_block_index;
      remove_block.next     = free_block_index_; // note here member variable
      stack_.Set(remove_index, remove_block);

      free_block_index = remove_index;
      free_blocks_added++;
      remove_index = next_remove_index;
    }
  }
}

/**
 * Verify that the file is consistent, given a list of all of the IDs
 *
 * @param: remove_index Index of starting block to free
 */
template <typename S>
bool FileObject<S>::VerifyConsistency(std::vector<uint64_t> const &ids)
{
  if(stack_.size() == 0)
  {
    if(ids.size() != 0)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Stack size is 0 but attempting to verify with ids");
      return false;
    }

    return true;
  }

  std::vector<uint64_t> fake_stack(stack_.size(), std::numeric_limits<uint64_t>::max());

  block_type block;
  uint64_t index = free_block_index_;

  // Verify our linked lists. Tuple = (index, prev, next)
  std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> linked_list;

  auto VerifyLL = [](std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> const &ll, bool circular) -> bool
  {
    bool ret = true;

    for (std::size_t i = 0; i < ll.size(); ++i)
    {
      uint64_t next_in_ll = std::get<0>(ll[(i + 1) % ll.size()]);
      uint64_t prev_in_ll = std::get<0>(ll[(i + ll.size() - 1) % ll.size()]);

      if(!circular)
      {
        next_in_ll = (i == ll.size() - 1) ? block_type::UNDEFINED : next_in_ll;
        prev_in_ll = (i == 0)             ? block_type::UNDEFINED : prev_in_ll;
      }

      if(std::get<1>(ll[i]) != prev_in_ll || std::get<2>(ll[i]) != next_in_ll)
      {
        ret = false;
        break;
      }
    }

    return ret;
  };

  // First traverse the free LL, ensure it is correct etc.
  stack_.Get(index, block);
  fake_stack[index] = 0;

  if(block.free_blocks == 0)
  {
    if(block.next != index || block.previous != index)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Free block is malformed - size is 0 but prev and next pointers don't refer back to itself.");
      return false;
    }
  }
  else
  {
    uint64_t free_blocks = block.free_blocks;

    for (uint64_t i = 0; i < free_blocks; ++i)
    {
      fake_stack[index] = 0;
      linked_list.push_back({index, block.previous, block.next});
      index = block.next;
      stack_.Get(index, block);
    }

    if(!VerifyLL(linked_list, true))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to verify free LL");
      return false;
    }
  }

  for(auto const &id : ids)
  {
    linked_list.clear();
    index = id;

    stack_.Get(index, block);

    uint64_t expected_blocks = platform::DivideCeil<uint64_t>(block.file_object_size, block_type::CAPACITY);
    expected_blocks = expected_blocks == 0 ? 1 : expected_blocks;

    for (uint64_t i = 0; i < expected_blocks; ++i)
    {
      fake_stack[index] = id;
      linked_list.push_back({index, block.previous, block.next});
      index = block.next;
      stack_.Get(index, block);
    }

    if(!VerifyLL(linked_list, false))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to verify LL for ID: ", id);
      return false;
    }
  }

  for (std::size_t i = 0; i < fake_stack.size(); ++i)
  {
    if(fake_stack[i] == std::numeric_limits<uint64_t>::max())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Found unaccounted for block in fake stack. Location: ", i);
      return false;
    }
  }

  return true;
}

}  // namespace storage
}  // namespace fetch
