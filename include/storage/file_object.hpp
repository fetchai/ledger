#ifndef STORAGE_FILE_OBJECT_HPP
#define STORAGE_FILE_OBJECT_HPP

namespace fetch {
namespace storage {

class FileObject {
 public:
  enum { UNDEFINED = uint64_t(-1), BYTES = 8 };

  struct BlockType {
    BlockType() {
      memset(this, 0, sizeof(decltype(*this)));
      previous = UNDEFINED;
      next = UNDEFINED;
    }

    uint64_t previous = UNDEFINED;
    uint64_t next = UNDEFINED;

    uint8_t data[BYTES];
  };

  typedef VersionedRandomAccessStack<BlockType> stack_type;

  FileObject(uint64_t const &position, stack_type &stack)
      : file_position_(position), stack_(stack) {}

  void Seek(std::size_t const &n) {
    uint64_t current_block_index = file_position_;
    BlockType block;

    int64_t remain = n;

    // Searching forward for block
    stack_.Get(current_block_index, block);
    assert(block.previous == UNDEFINED);
    while (rem >= BYTES) {
      if (block.next == UNDEFINED) {
        block.next = stack_.size();
        stack_.Set(current_block_index, block);
        block = BlockType();
        block.previous = current_block_index;
        stack_.Push(block);
      } else {
        b = block.next;
      }

      stack_.Get(current_block_index, block);
      remain -= BYTES;
    }

    block_index_ = current_block_index;
    byte_index_ = remain;
  }

  std::size_t Tell() const { return byte_index_ + block_index_ * BYTES; }

  void Write(uint8_t const *bytes, std::size_t const &n) {
    std::size_t i = 0;
    BlockType block;
    stack_.Get(block_index_, block);

    while (i < n) {
      block.data[byte_index_] = bytes[i];
      ++i, ++byte_index_;
      GetOrExpand(block);
    }
  }

  void Read(uint8_t *bytes, std::size_t const &n) {
    std::size_t i = 0;
    BlockType block;
    stack_.Get(block_index_, block);

    while (i < n) {
      bytes[i] = block.data[byte_index_];
      ++i, ++byte_index_;
      GetOrExpand(block);
    }
  }

  uint64_t Size() const {}

  uint64_t Shrink() {}

  uint64_t const &file_position() const { return file_position_; }

 private:
  void GetOrExpand(BlockType &block) {
    if (byte_index_ == BYTES) {
      stack_.Set(block_index_, block);
      block_index_ = block.next;
      if (block_index_ == UNDEFINED) {
        block.next = stack_.size();
        stack.Set(block_index_, block);

        block = BlockType();
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
};
};

#endif
