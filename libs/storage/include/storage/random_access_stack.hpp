#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

//  ┌──────┬───────────┬───────────┬───────────┬───────────┐
//  │      │           │           │           │           │
//  │HEADER│  OBJECT   │  OBJECT   │  OBJECT   │  OBJECT   │
//  │      │           │           │           │           │......
//  │      │           │           │           │           │
//  └──────┴───────────┴───────────┴───────────┴───────────┘

#include <cassert>
#include <fstream>
#include <functional>
#include <string>

#include "core/assert.hpp"

namespace fetch {
namespace platform {
enum
{
  LITTLE_ENDIAN_MAGIC = 2337
};
}
namespace storage {

/**
 * The RandomAccessStack maintains a stack of type T, writing to disk. Since elements on the stack
 * are uniform size, they can be easily addressed using simple arithmetic.
 *
 * The header for the stack optionally allows arbitrary data to be stored, which can be useful to
 * the user
 */
template <typename T, typename D = uint64_t>
class RandomAccessStack
{
private:

  /**
   * Header holding information for the structure. Magic is used to determine the endianness of the
   * platform, extra allows the user to write metadata for the structure. This is used for example
   * in key value store to store the head of the trie
   */
  struct Header
  {
    uint16_t magic   = platform::LITTLE_ENDIAN_MAGIC;
    uint64_t objects = 0;
    D        extra;

    bool Write(std::fstream &stream) const
    {
      if ((!stream) || (!stream.is_open())) return false;
      stream.seekg(0, stream.beg);
      stream.write(reinterpret_cast<char const *>(&magic), sizeof(magic));
      stream.write(reinterpret_cast<char const *>(&objects), sizeof(objects));
      stream.write(reinterpret_cast<char const *>(&extra), sizeof(extra));
      return bool(stream);
    }

    bool Read(std::fstream &stream)
    {
      if ((!stream) || (!stream.is_open())) return false;
      stream.seekg(0, stream.beg);
      stream.read(reinterpret_cast<char *>(&magic), sizeof(magic));
      stream.read(reinterpret_cast<char *>(&objects), sizeof(objects));
      stream.read(reinterpret_cast<char *>(&extra), sizeof(extra));
      return bool(stream);
    }

    constexpr std::size_t size() const { return sizeof(magic) + sizeof(objects) + sizeof(D); }
  };

public:
  using header_extra_type  = D;
  using type               = T;
  using event_handler_type = std::function<void()>;

  void ClearEventHandlers()
  {
    on_file_loaded_  = nullptr;
    on_before_flush_ = nullptr;
  }

  void OnFileLoaded(event_handler_type const &f) { on_file_loaded_ = f; }

  void OnBeforeFlush(event_handler_type const &f) { on_before_flush_ = f; }

  void SignalFileLoaded()
  {
    if (on_file_loaded_) on_file_loaded_();
  }

  void SignalBeforeFlush()
  {
    if (on_before_flush_) on_before_flush_();
  }

  /**
   * Indicate whether the stack is writing directly to disk or caching writes.
   *
   * @return: Whether the stack is written straight to disk.
   */
  static constexpr bool DirectWrite() { return true; }

  ~RandomAccessStack()
  {
    if (file_handle_.is_open())
    {
      file_handle_.close();
    }
  }

  void Close(bool const &lazy = false)
  {
    if (!lazy) Flush();
    file_handle_.close();
  }

  void Load(std::string const &filename, bool const &create_if_not_exist = false)
  {
    filename_    = filename;
    file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);

    if (!file_handle_)
    {
      if (create_if_not_exist)
      {
        Clear();
        file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
      }
      else
      {
        TODO_FAIL("Could not load file");
      }
    }

    // Get length of file
    file_handle_.seekg(0, file_handle_.end);
    int64_t length = file_handle_.tellg();

    // Read the beginning of the file into our header
    file_handle_.seekg(0, file_handle_.beg);
    header_.Read(file_handle_);

    int64_t capacity = (length - int64_t(header_.size())) / int64_t(sizeof(type));
    assert(capacity >= 0);

    if (std::size_t(capacity) < header_.objects)
    {
      TODO_FAIL("Expected more stack objects.");
    }

    // TODO(issue 6): Check magic

    SignalFileLoaded();
  }

  void New(std::string const &filename)
  {
    filename_ = filename;
    Clear();
    file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);

    SignalFileLoaded();
  }

  // TODO(issue 6): Protected functions
  /**
   * Get object on the stack at index i, not safe when i > objects.
   *
   * @param: i The Ith object, indexed from 0
   * @param: object The object reference to fill
   *
   */
  void Get(std::size_t const &i, type &object) const
  {
    assert(filename_ != "");
    assert(i < size());

    int64_t n = int64_t(i * sizeof(type) + header_.size());

    file_handle_.seekg(n);
    file_handle_.read(reinterpret_cast<char *>(&object), sizeof(type));
  }

  /**
   * Set object on the stack at index i, not safe when i > objects.
   *
   * @param: i The Ith object, indexed from 0
   * @param: object The object to copy to the stack
   *
   */
  void Set(std::size_t const &i, type const &object)
  {
    assert(filename_ != "");
    assert(i < size());
    int64_t n = int64_t(i * sizeof(type) + header_.size());

    file_handle_.seekg(n, file_handle_.beg);
    file_handle_.write(reinterpret_cast<char const *>(&object), sizeof(type));
  }

  void SetExtraHeader(header_extra_type const &he)
  {
    assert(filename_ != "");

    header_.extra = he;
    StoreHeader();
  }

  header_extra_type const &header_extra() const { return header_.extra; }

  /**
   * Push a new object onto the stack, increasing its size by one.
   *
   * Note: also writes the header to disk, increasing access time. Alternatively use LazyPush
   *
   * @param: object The object to push
   *
   * @return: the number of objects on the stack
   */
  uint64_t Push(type const &object)
  {
    uint64_t ret = header_.objects;
    int64_t  n   = int64_t(ret * sizeof(type) + header_.size());

    file_handle_.seekg(n, file_handle_.beg);
    file_handle_.write(reinterpret_cast<char const *>(&object), sizeof(type));

    ++header_.objects;
    StoreHeader();
    return ret;
  }

  /**
   * Remove the top element of the stack. Not safe when the stack has no objects.
   */
  void Pop()
  {
    --header_.objects;
    StoreHeader();
  }

  /**
   * Return the object at the top of the stack. Not safe when the stack has no objects.
   *
   * @return: the object at the top of the stack.
   */
  type Top() const
  {
    assert(header_.objects > 0);

    int64_t n = int64_t((header_.objects - 1) * sizeof(type) + header_.size());

    file_handle_.seekg(n, file_handle_.beg);
    type object;
    file_handle_.read(reinterpret_cast<char *>(&object), sizeof(type));

    return object;
  }

  /**
   * Swap the objects at two locations on the stack. Must be valid locations.
   *
   * @param: i Location of the first object
   * @param: j Location of the second object
   *
   */
  void Swap(std::size_t const &i, std::size_t const &j)
  {
    if (i == j) return;
    type a, b;
    assert(filename_ != "");

    int64_t n1 = int64_t(i * sizeof(type) + header_.size());
    int64_t n2 = int64_t(j * sizeof(type) + header_.size());

    file_handle_.seekg(n1);
    file_handle_.read(reinterpret_cast<char *>(&a), sizeof(type));
    file_handle_.seekg(n2);
    file_handle_.read(reinterpret_cast<char *>(&b), sizeof(type));

    file_handle_.seekg(n1);
    file_handle_.write(reinterpret_cast<char const *>(&b), sizeof(type));
    file_handle_.seekg(n2);
    file_handle_.write(reinterpret_cast<char const *>(&a), sizeof(type));
  }

  std::size_t size() const { return header_.objects; }

  std::size_t empty() const { return header_.objects == 0; }

  /**
   * Clear the file and write an 'empty' header to the file
   */
  void Clear()
  {
    assert(filename_ != "");
    std::fstream fin(filename_, std::ios::out | std::ios::binary);
    header_ = Header();

    if (!header_.Write(fin))
    {
      TODO_FAIL("Error could not write header - todo throw error");
    }

    fin.close();
  }

  /**
   * Flushing writes the header to disk - there isn't necessarily any need to keep writing to disk
   * with every push etc.
   *
   * @param: lazy Whether to execute user defined callbacks
   */
  void Flush(bool const &lazy = false)
  {
    if (!lazy) SignalBeforeFlush();
    StoreHeader();
    file_handle_.flush();
  }

  bool is_open() const { return bool(file_handle_) && (file_handle_.is_open()); }

  /**
   * Push only the object to disk, this requires the user to flush the header before file closure
   * to avoid corrupting the file
   *
   * @param: object The object to write
   */
  uint64_t LazyPush(type const &object)
  {
    uint64_t ret = header_.objects;
    int64_t  n   = int64_t(ret * sizeof(type) + header_.size());

    file_handle_.seekg(n, file_handle_.beg);
    file_handle_.write(reinterpret_cast<char const *>(&object), sizeof(type));
    ++header_.objects;

    return ret;
  }

private:
  event_handler_type   on_file_loaded_;
  event_handler_type   on_before_flush_;
  mutable std::fstream file_handle_;
  std::string          filename_ = "";
  Header               header_;

  /**
   * Write the header to disk. Not usually necessary since we can just refer to our local one
   *
   * @param: 
   *
   * @return: 
   */
  void StoreHeader()
  {
    assert(filename_ != "");

    if (!header_.Write(file_handle_))
    {
      TODO_FAIL("Error could not write header - todo throw error");
    }
  }

};
}  // namespace storage
}  // namespace fetch
