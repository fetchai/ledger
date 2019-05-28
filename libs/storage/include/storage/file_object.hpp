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

//    ┌────────────────────────────────────────────────┐
//    │      ┌──────┬────────────────────┐             │
//    │      │      │                    │             │
//    ▼      │      │                    ▼             │
// ┌──────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┐
// │ FREE │Obj1.1│Obj1.2│ FREE │Obj2.1│Obj1.3│Obj2.2│ FREE │
// │  LL  │      │      │      │      │      │      │      │
// └──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┘
//    │                    ▲      │             ▲      ▲
//    │                    │      │             │      │
//    │                    │      └─────────────┘      │
//    │                    │                           │
//    └────────────────────┴───────────────────────────┘

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
  static constexpr uint64_t META_DATA_BYTES = (3 * sizeof(uint64_t));
  static constexpr uint64_t CAPACITY        = BS - META_DATA_BYTES;
  static constexpr uint64_t UNDEFINED       = std::numeric_limits<uint64_t>::max();

  static_assert(BS > META_DATA_BYTES,
                "Block size needs to exceed the min requirement for metadata");

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
  uint64_t previous = UNDEFINED;  // For the free block, this points to the end of the LL.

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

  FileObject(FileObject const &other) = delete;
  FileObject operator=(FileObject const &other) = delete;
  FileObject(FileObject &&other)                = default;
  FileObject &operator=(FileObject &&other) = default;

  FileObject();
  virtual ~FileObject();

  // TODO(private 1067): Unify this new/load methodology to avoid templates
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

  uint64_t Tell() const;

  void Resize(uint64_t size);

  void Write(byte_array::ConstByteArray const &arr);

  void Write(uint8_t const *bytes, uint64_t const &num);

  void Read(byte_array::ByteArray &arr);

  void Read(uint8_t *bytes, uint64_t const &m);

  uint64_t const &id() const;

  uint64_t FileObjectSize() const;

  byte_array::ConstByteArray Hash();

  void UpdateHash(crypto::StreamHasher &hasher);

  bool SeekFile(std::size_t const &position);

  void CreateNewFile(uint64_t size = 0);

  Document AsDocument();

  void Erase();

  bool VerifyConsistency(std::vector<uint64_t> const &ids);

  stack_type *stack();
  stack_type &underlying_stack();

  void UpdateVariables()
  {
    block_index_ = 0;
    id_          = 0;
  }

private:
  stack_type stack_;

  std::vector<std::vector<std::tuple<uint64_t, uint64_t, uint64_t>>> linked_lists;

  // tracking variables relating to current file object
  uint64_t id_                = 0;  // Location on stack of first block of file
  uint64_t block_number_      = 0;  // Nth block in file we are looking at
  uint64_t block_index_       = 0;  // index of block_number_ on the stack
  uint64_t byte_index_        = 0;  // index of current byte within BLOCK
  uint64_t byte_index_global_ = 0;  // index of current byte within file
  uint64_t length_            = 0;  // length in bytes of file.
                                    // can be found from Get(id) right - any point in keeping?

  // TODO(private 1067): block_type -> BlockType etc.
  // TODO(private 1067): possibly some performance benefits by caching blocks like the free block
  // here
  static constexpr uint64_t free_block_index_ = 0;  // Location of the meta 'free block'

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

  void Get(uint64_t index, block_type &block);

  void Set(uint64_t index, block_type const &block);
};

template <typename S>
FileObject<S>::FileObject() = default;

template <typename S>
FileObject<S>::~FileObject()
{
  // TODO(private 1067): consistency in storage : don't flush in destructors, except the highest
  // level storage code.
}

template <typename S>
void FileObject<S>::Flush(bool lazy)
{
  stack_.Flush(lazy);
}

template <typename S>
void FileObject<S>::Seek(uint64_t index)
{
  byte_index_global_    = index;
  byte_index_           = index % block_type::CAPACITY;
  uint64_t   desired_bn = index / block_type::CAPACITY;
  block_type block;

  // Optionally start seeking from the first block if that's faster
  if (desired_bn < block_number_ && block_number_ - desired_bn < desired_bn)
  {
    block_number_ = 0;
  }

  // need to find index of this block on stack
  while (block_number_ != desired_bn)
  {
    if (block_number_ == block_type::UNDEFINED)
    {
      throw StorageException("Attempt to seek to an invalid block");
    }

    Get(block_number_, block);

    if (desired_bn < block_number_)
    {
      block_number_ = block.previous;
    }
    else
    {
      block_number_ = block.next;
    }
  }
}

// TODO(private 1067): make sure everything is const correct
template <typename S>
uint64_t FileObject<S>::Tell() const
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
    Get(id_, block);

    block.file_object_size = size;
    Set(id_, block);
    length_ = size;
  }

  // Traverse the blocks until the target blocks criteria is fulfilled.
  uint64_t target_blocks = platform::DivideCeil<uint64_t>(size, block_type::CAPACITY);

  // corner case when size is 0 - we need at least one block per file
  target_blocks = target_blocks == 0 ? 1 : target_blocks;

  uint64_t block_number = 1;
  uint64_t block_index  = id_;

  for (;;)
  {
    Get(block_index, block);

    uint64_t block_index_tmp = block_index;
    block_index              = block.next;

    // if we reach the end of the LL, append as many blocks as we need
    if (block.next == block_type::UNDEFINED && block_number != target_blocks)
    {
      block.next = GetFreeBlocks(block_index_tmp, target_blocks - block_number);
      Set(block_index_tmp, block);

      uint64_t next_tmp = block.next;

      // need to update the new blocks' backward references too
      Get(next_tmp, block);
      block.previous = block_index_tmp;
      Set(next_tmp, block);
      break;
    }

    // If we reach as many blocks we actually need, free the rest
    if (block_number == target_blocks)
    {
      FreeBlocksInList(block_index);

      // Update last block to terminate correctly
      Get(block_index_tmp, block);
      block.next = block_type::UNDEFINED;
      Set(block_index_tmp, block);
      break;
    }

    block_number++;
  }
}

template <typename S>
void FileObject<S>::Write(byte_array::ConstByteArray const &arr)
{
  Write(arr.pointer(), arr.size());
}

/**
 * Write bytes to file object, ensuring that the size of the file object is extended if
 * necessary.
 *
 * @param: bytes The bytes to write
 * @param: num The number of bytes to write
 *
 */
template <typename S>
void FileObject<S>::ReadWriteHelper(uint8_t const *bytes, uint64_t const &num, Action action)
{
  assert(id_ != 0 &&
         "Attempt to write to the free block as if it were a file is a programmer error in "
         "FileObject");

  uint64_t bytes_left_to_write = num;
  uint64_t bytes_to_write_in_block =
      std::min(block_type::CAPACITY - byte_index_, bytes_left_to_write);
  block_type block_being_written;
  uint64_t   block_index_being_written = block_index_;
  uint64_t   byte_index                = byte_index_;
  uint64_t   bytes_offset              = 0;

  assert(bytes_to_write_in_block <= block_type::CAPACITY);

  while (bytes_left_to_write > 0)
  {
    // TODO(private 1067): use max size type
    assert(block_index_being_written != uint64_t(-1) && "Invalid block index during writing");
    // Get block, write data to it
    Get(block_index_being_written, block_being_written);

    switch (action)
    {
    case Action::READ:
      memcpy((uint8_t *)(bytes + bytes_offset), block_being_written.data + byte_index,
             bytes_to_write_in_block);
      break;
    case Action::WRITE:
      memcpy(block_being_written.data + byte_index, bytes + bytes_offset, bytes_to_write_in_block);
      break;
    }

    // Write block back
    byte_index = 0;
    Set(block_index_being_written, block_being_written);
    block_index_being_written = block_being_written.next;

    bytes_offset += bytes_to_write_in_block;
    bytes_left_to_write -= bytes_to_write_in_block;
    bytes_to_write_in_block = std::min(block_type::CAPACITY - byte_index_, bytes_left_to_write);
  }
}

// TODO(private 1067): num -> bytes_left_to_write renaming consistency
template <typename S>
void FileObject<S>::Write(uint8_t const *bytes, uint64_t const &num)
{
  ReadWriteHelper(bytes, num, Action::WRITE);
}

template <typename S>
void FileObject<S>::Read(byte_array::ByteArray &arr)
{
  Read(arr.pointer(), arr.size());
}

// TODO(private 1067): rename m -> num
template <typename S>
void FileObject<S>::Read(uint8_t *bytes, uint64_t const &m)
{
  ReadWriteHelper(bytes, m, Action::READ);
}

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

template <typename S>
byte_array::ConstByteArray FileObject<S>::Hash()
{
  hasher_type hasher;
  hasher.Reset();
  UpdateHash(hasher);

  return hasher.Final();
}

template <typename S>
void FileObject<S>::UpdateHash(crypto::StreamHasher &hasher)
{
  Seek(0);
  byte_array::ByteArray arr;
  arr.Resize(length_);
  Read(arr);

  std::string thing = std::string{arr};
  FETCH_UNUSED(thing);

  hasher.Update(arr.pointer(), length_);
}

template <typename S>
bool FileObject<S>::SeekFile(std::size_t const &position)
{
  if (position >= stack_.size())
  {
    throw StorageException("Attempt to seek file past stack end");
  }

  block_index_ = id_ = position;
  block_number_      = 0;
  byte_index_        = 0;
  byte_index_global_ = 0;

  // Need to retrieve the block to determine length
  block_type block;
  Get(id_, block);

  length_ = block.file_object_size;

  return true;
}

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

  block_type block;
  Get(id_, block);
  block.file_object_size = size;
  Set(id_, block);
}

template <typename S>
Document FileObject<S>::AsDocument()
{
  Document ret;

  if (byte_index_global_ > length_)
  {
    throw StorageException(
        "global byte index exceeds length when attempting to retrieve file object");
  }

  ret.document.Resize(length_ - byte_index_global_);
  this->Read(ret.document);

  return ret;
}

template <typename S>
void FileObject<S>::Erase()
{
  FreeBlocksInList(id_);
  id_     = std::numeric_limits<uint64_t>::max();
  length_ = 0;
}

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
  Get(free_block_index_, free_block);

  return free_block.free_blocks;
}

/**
 * Initialise by looking for the block that's the beginning of our free blocks linked list. If
 * the stack is empty this means we set our own.
 */
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
  if (num == 0)
  {
    throw StorageException("Attempt to get 0 free blocks is invalid");
  }

  auto DefaultFreeAllocation = [this](uint64_t num) {
    if (num == 0)
    {
      throw StorageException("Attempt to allocate 0 free blocks is invalid");
    }

    block_type block;
    uint64_t   first_block_index = stack_.size();

    for (uint64_t i = 0; i < num; ++i)
    {
      block.previous = (i == 0) ? block_type::UNDEFINED : first_block_index + i - 1;
      block.next     = (i == num - 1) ? block_type::UNDEFINED : first_block_index + i + 1;
      stack_.Push(block);
    }

    return first_block_index;
  };

  block_type free_block;
  block_type block;
  Get(free_block_index_, free_block);

  // Only allow free blocks to be taken from the free LL in the scenario that there are enough
  // blocks in the LL above min_index. This way it is easier to traverse from the end of the stack
  // backwards, then 'cut' the LL at that point. Check here for easy exit.
  if (free_block.free_blocks == 0 || free_block.previous < min_index ||
      free_block.free_blocks < num)
  {
    return DefaultFreeAllocation(num);
  }

  uint64_t index             = free_block.previous;  // Index of block in LL we wish to free
  uint64_t free_blocks_in_ll = 0;

  // Traverse backwards in the stack
  for (;;)
  {
    Get(index, block);
    free_blocks_in_ll++;

    if (index < min_index)
    {
      break;
    }

    if (index == free_block_index_)
    {
      break;
    }

    // This case has found it successfully
    if (free_blocks_in_ll == num)
    {
      // Point end of LL back to free block
      uint64_t index_prev = block.previous;
      uint64_t old_ll_end = free_block.previous;

      // For debugging - terminate this block
      {
        block.previous = block_type::UNDEFINED;
        Set(index, block);
      }

      Get(index_prev, block);
      block.next = free_block_index_;
      Set(index_prev, block);

      // Update free block - NEED get here as index_prev might be free block
      assert(free_block.free_blocks >= num);
      Get(free_block_index_, free_block);
      free_block.free_blocks -= num;
      free_block.previous = index_prev;
      Set(free_block_index_, free_block);

      // Last block in the free LL needs to terminate as it's now a file object
      Get(old_ll_end, block);
      block.next = block_type::UNDEFINED;
      Set(old_ll_end, block);

      return index;
    }

    index = block.previous;
  }

  // Failed to cut the stack.
  return DefaultFreeAllocation(num);
}

/**
 * Free blocks starting at index. We can't just append to the end of the free LL as we want to
 * maintain ordering.
 * Note that the block at remove_index must point to a valid LL, the last block of which points to
 * 'invalid'
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

  Get(free_block_index_, free_block);

  uint64_t free_block_index      = free_block_index_;
  uint64_t free_block_next_index = free_block.next;
  uint64_t free_blocks_added     = 0;

  bool insert_mode = true;

  while (remove_index != block_type::UNDEFINED)
  {
    Get(free_block_index, free_block);
    Get(free_block_next_index, free_block_next);
    Get(remove_index, remove_block);

    if (insert_mode)
    {
      // Reached the end of the LL and wrapped around
      if (free_block_next_index == 0)
      {
        insert_mode = false;
        continue;
      }

      // Insert the remove block iff it sits between the pointers
      if (remove_index > free_block_index && remove_index < free_block_next_index)
      {
        uint64_t next_remove_index = remove_block.next;

        // Zero the removed block
        remove_block = block_type{};

        // Adjust remove block references
        remove_block.previous = free_block_index;
        remove_block.next     = free_block_next_index;
        Set(remove_index, remove_block);

        // Adjust 'prev' block to this
        free_block.next = remove_index;
        Set(free_block_index, free_block);

        // Adjust 'next' block to this
        free_block_next.previous = remove_index;
        Set(free_block_next_index, free_block_next);

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
      Set(free_block_index, free_block);

      // Free LL is circular
      {
        Get(free_block_index_, free_block);
        free_block.previous = remove_index;
        Set(free_block_index_, free_block);
      }

      uint64_t next_remove_index = remove_block.next;

      remove_block          = block_type{};
      remove_block.previous = free_block_index;
      remove_block.next     = free_block_index_;  // note here member variable
      Set(remove_index, remove_block);

      free_block_index = remove_index;
      free_blocks_added++;
      remove_index = next_remove_index;
    }
  }

  // Finally update the free block with the new amount of free blocks added
  Get(free_block_index_, free_block);
  free_block.free_blocks += free_blocks_added;
  Set(free_block_index_, free_block);
}

/**
 * Verify that the file is consistent, given a list of all of the IDs
 *
 * @param: remove_index Index of starting block to free
 */
template <typename S>
bool FileObject<S>::VerifyConsistency(std::vector<uint64_t> const &ids)
{
  if (stack_.size() == 0)
  {
    if (ids.size() != 0)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Stack size is 0 but attempting to verify with ids");
      return false;
    }

    return true;
  }

  std::vector<uint64_t> fake_stack(stack_.size(), std::numeric_limits<uint64_t>::max());

  block_type block;
  uint64_t   index = free_block_index_;

  // Verify our linked lists. Tuple = (index, prev, next)
  std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> linked_list;

  auto VerifyLL = [](std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> const &ll,
                     bool circular) -> bool {
    for (std::size_t i = 0; i < ll.size(); ++i)
    {
      uint64_t indx = std::get<0>(ll[i]);
      uint64_t prev = std::get<1>(ll[i]);
      uint64_t next = std::get<2>(ll[i]);

      uint64_t next_in_ll = std::get<0>(ll[(i + 1) % ll.size()]);
      uint64_t prev_in_ll = std::get<0>(ll[(i + ll.size() - 1) % ll.size()]);

      if (indx == block_type::UNDEFINED)
      {
        return false;
      }

      // Check LL consistently increments except at the boundaries.
      if (!(next > indx))
      {
        // Corner case at end of list
        if (!(i == ll.size() - 1))
        {
          return false;
        }
      }

      if (!(prev < indx))
      {
        // Corner case at beginning of list
        if (!(i == 0))
        {
          return false;
        }
      }

      if (!circular)
      {
        next_in_ll = (i == ll.size() - 1) ? block_type::UNDEFINED : next_in_ll;
        prev_in_ll = (i == 0) ? block_type::UNDEFINED : prev_in_ll;
      }

      if (prev != prev_in_ll || next != next_in_ll)
      {
        return false;
      }
    }

    return true;
  };

  // First traverse the free LL, ensure it is correct etc.
  Get(index, block);
  fake_stack[index] = 0;

  if (block.free_blocks == 0)
  {
    if (block.next != index || block.previous != index)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Free block is malformed - size is 0 but prev and next pointers don't refer "
                      "back to itself.");
      return false;
    }
  }
  else
  {
    uint64_t free_blocks = block.free_blocks;

    // Plus one for free block
    for (uint64_t i = 0; i < free_blocks + 1; ++i)
    {
      fake_stack[index] = 0;
      linked_list.push_back({index, block.previous, block.next});
      index = block.next;
      Get(index, block);
    }

    linked_lists.push_back(linked_list);

    if (!VerifyLL(linked_list, true))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to verify free LL");
      return false;
    }
  }

  for (auto const &id : ids)
  {
    linked_list.clear();
    index = id;

    Get(index, block);

    uint64_t file_bytes      = block.file_object_size;
    uint64_t expected_blocks = platform::DivideCeil<uint64_t>(file_bytes, block_type::CAPACITY);
    expected_blocks          = expected_blocks == 0 ? 1 : expected_blocks;

    for (uint64_t i = 0; i < expected_blocks; ++i)
    {
      fake_stack[index] = id;
      linked_list.push_back({index, block.previous, block.next});
      index = block.next;

      if (i + 1 < expected_blocks)
      {
        Get(index, block);
      }
    }

    if (!VerifyLL(linked_list, false))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to verify LL for ID: ", id);
      return false;
    }
  }

  for (std::size_t i = 0; i < fake_stack.size(); ++i)
  {
    if (fake_stack[i] == std::numeric_limits<uint64_t>::max())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Found unaccounted for block in fake stack. Location: ", i);
      return false;
    }
  }

  return true;
}

template <typename S>
void FileObject<S>::Get(uint64_t index, block_type &block)
{
  if (index == block_type::UNDEFINED)
  {
    throw StorageException("Attempt to Get invalid location");
  }

  stack_.Get(index, block);
}

template <typename S>
void FileObject<S>::Set(uint64_t index, block_type const &block)
{
  if (index == block_type::UNDEFINED)
  {
    throw StorageException("Attempt to Set invalid location");
  }

  stack_.Set(index, block);
}

}  // namespace storage
}  // namespace fetch
