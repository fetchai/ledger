#ifndef STORAGE_FILE_OBJECT_HPP
#define STORAGE_FILE_OBJECT_HPP
#include "storage/versioned_random_access_stack.hpp"

#include <cstdint>

namespace fetch {
namespace storage {
namespace details {
struct StorageBlockType {
  enum { BYTES = 8, UNDEFINED = uint64_t(-1) };

  StorageBlockType() {
    // Ensures that paddded bytes are not uninitialized.
    memset(this, 0, sizeof(decltype(*this)));
    previous = UNDEFINED;
    next = UNDEFINED;
  }

  uint64_t previous = UNDEFINED;
  uint64_t next = UNDEFINED;
  uint8_t data[BYTES];
};
};

template <typename S = VersionedRandomAccessStack<details::StorageBlockType> >
class FileObjectImplementation {
 public:
  typedef S stack_type;
  typedef typename stack_type::type block_type;

  FileObjectImplementation(uint64_t const &block_position, stack_type &stack)
      : file_position_(block_position), stack_(stack) {}

  void Seek(int64_t const &n) {
    uint64_t current_block_index = file_position_;
    block_type block;

    int64_t remain = n;

    // Searching forward for block
    stack_.Get(current_block_index, block);
    assert(block.previous == block_type::UNDEFINED);
    while (remain >= block_type::BYTES) {
      if (block.next == block_type::UNDEFINED) {
        block.next = stack_.size();
        stack_.Set(current_block_index, block);
        block = block_type();
        block.previous = current_block_index;
        stack_.Push(block);
      } else {
        auto b = block.next;
        (void) b;
      }

      stack_.Get(current_block_index, block);
      remain -= block_type::BYTES;
    }

    block_index_ = current_block_index;
    byte_index_ = remain;
  }

  std::size_t Tell() const {
    return byte_index_ + block_index_ * block_type::BYTES;
  }

  void Write(uint8_t const *bytes, std::size_t const &n) {
    std::size_t i = 0;
    block_type block;
    stack_.Get(block_index_, block);

    while (i < n) {
      block.data[byte_index_] = bytes[i];
      ++i, ++byte_index_;
      GetOrExpand(block);
    }
  }

  void Read(uint8_t *bytes, std::size_t const &n) {
    std::size_t i = 0;
    block_type block;
    stack_.Get(block_index_, block);

    while (i < n) {
      bytes[i] = block.data[byte_index_];
      ++i, ++byte_index_;
      GetOrExpand(block);
    }
  }

  uint64_t Size() const { return 0; }  // TODO

  uint64_t Shrink() { return 0; }  // TODO

  uint64_t const &file_position() const { return file_position_; }

 private:
  void GetOrExpand(block_type &block) {
    if (byte_index_ == block_type::BYTES) {
      stack_.Set(block_index_, block);
      block_index_ = block.next;

      if (block_index_ == block_type::UNDEFINED) {
        block.next = stack_.size();
        stack_.Set(block_index_, block);

        block = block_type();
        block.previous = block_index_;
        block_index_ = stack_.size();
        stack_.Push(block);

      } else {
        stack_.Get(block_index_, block);
      }
      byte_index_ = 0;
    }
  }

  uint64_t file_position_;
  stack_type &stack_;

  uint64_t block_index_;
  uint64_t byte_index_;
};
}
}

#endif
