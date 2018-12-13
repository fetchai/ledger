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

#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <storage/fetch_mmap.hpp>
#include <string>

#include "core/assert.hpp"
#include "storage/storage_exception.hpp"

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
 * Note that objects are required to be the same size. This means you should not store classes with
 * dynamically allocated memory.
 *
 * The header for the stack optionally allows arbitrary data to be stored, which can be useful to
 * the user
 */

template <typename T, typename D = uint64_t, unsigned long MAX = 256>
class RandomAccessStackMMap
{
private:
  static constexpr char const *LOGGING_NAME = "RandomAccessStackMMap";

/**
 * Header holding information for the structure. Magic is used to determine the endianness of the
 * platform, extra allows the user to write metadata for the structure. This is used for example
 * in key value store to store the head of the trie
 */
#pragma pack(push, 1)
  struct Header
  {
    uint16_t    magic   = platform::LITTLE_ENDIAN_MAGIC;
    uint64_t    objects = 0;
    D           extra;
    std::size_t size() const
    {
      return sizeof(magic) + sizeof(objects) + sizeof(extra);
    }
    bool Write(std::fstream &stream) const
    {

      if ((!stream) || (!stream.is_open()))
      {
        return false;
      }
      stream.seekg(0, stream.beg);
      stream.write(reinterpret_cast<char const *>(&magic), sizeof(magic));
      stream.write(reinterpret_cast<char const *>(&objects), sizeof(objects));
      stream.write(reinterpret_cast<char const *>(&extra), sizeof(extra));
      stream.flush();
      return bool(stream);
    }
  };
#pragma pack(pop)
public:
  using header_extra_type  = D;
  using type               = T;
  using event_handler_type = std::function<void()>;

  void ClearEventHandlers()
  {
    on_file_loaded_  = nullptr;
    on_before_flush_ = nullptr;
  }

  void OnFileLoaded(event_handler_type const &f)
  {
    on_file_loaded_ = f;
  }

  void OnBeforeFlush(event_handler_type const &f)
  {
    on_before_flush_ = f;
  }

  void SignalFileLoaded()
  {
    if (on_file_loaded_)
    {
      on_file_loaded_();
    }
  }

  void SignalBeforeFlush()
  {
    if (on_before_flush_)
    {
      on_before_flush_();
    }
  }

  /**
   * Indicate whether the stack is writing directly to disk or caching writes. Note the stack
   * will not flush on destruction.
   *
   * @return: Whether the stack is written straight to disk.
   */
  static constexpr bool DirectWrite()
  {
    return true;
  }

  ~RandomAccessStackMMap()
  {
    if (file_handle_.is_open())
    {
      Flush();
      mapped_data_.unmap();
      mapped_header_.unmap();
      file_handle_.close();
    }
  }
  ////////////////////
  void Close(bool const &lazy = false)
  {
    if (!lazy)
    {
      Flush();
    }
    mapped_data_.unmap();
    mapped_header_.unmap();
    file_handle_.close();
  }

  /////////////////////////
  void Load(std::string const &filename, bool const &create_if_not_exist = false)
  {
    bool is_initialized = false;
    filename_           = filename;
    file_handle_        = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_handle_)
    {
      if (create_if_not_exist)
      {
        New(filename_);
        is_initialized = true;
      }
      else
      {
        throw StorageException("Could not load file");
      }
    }
    if (!is_initialized)
    {
      InitializeMapping();
    }
    SignalFileLoaded();
  }

  //////////////////////////////////
  void New(std::string const &filename)
  {
    filename_ = filename;
    file_handle_ =
        std::fstream(filename_, std::fstream::in | std::fstream::out | std::fstream::binary);
    if (file_handle_.is_open())
    {
      Clear();
    }
    else
    {
      throw StorageException("Could not open file");
    }
    ResizeFile();
    InitializeMapping();
    SignalFileLoaded();
  }
  /////////////////////////////////////
  void Get(std::size_t const &i, type &object)
  {
    assert(filename_ != "");
    assert(i < size());
    if (!IsMapped(i))
    {
      MapIndex(i);
    }

    std::size_t pointer_offset = i - mapped_index_;
    type *      temp_Obj       = reinterpret_cast<type *>(mapped_data_.data());
    memcpy((&object), &(temp_Obj[pointer_offset]), sizeof(object));
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
    assert(i <= size());
    if (!IsMapped(i))
    {
      MapIndex(i);
    }

    std::size_t pointer_offset = i - mapped_index_;
    type *      temp_Obj       = reinterpret_cast<type *>(mapped_data_.data());
    memcpy(&(temp_Obj[pointer_offset]), &object, sizeof(type));
  }

  /**
   * Copy array of objects onto the stack, don't respect current stack size, just
   * update it if neccessary.
   *
   * @param: i Location of first object to be written
   * @param: elements Number of elements to copy
   * @param: objects Pointer to array of elements
   *
   */
  void SetBulk(std::size_t const &i, std::size_t elements, type *objects)
  {
    assert(filename_ != "");
    std::error_code error;
    size_t          capacity = 0, curr_in = i, elm_mapped = 0, elm_left = elements;
    size_t          pos, pointer_offset = 0;
    while (elm_left > 0)
    {
      if (!IsMapped(curr_in))
      {
        MapIndex(curr_in);
      }
      capacity = (mapped_index + MAX) - curr_in;
      if (elm_left <= capacity)
      {
        elm_mapped = elm_left;
        elm_left   = 0;
      }
      else
      {
        elm_mapped = capacity;
        elm_left   = elm_left - elm_mapped;
      }
      pos                 = curr_in - mapped_index_;
      type *file_offset   = (reinterpret_cast<type *>(mapped_data_.data())) + pos;
      type *object_offset = objects + pointer_offset;
      memcpy((file_offset), (object_offset), sizeof(type) * elm_mapped);
      // index will be incremented by number of written object
      curr_in += elm_mapped;
      pointer_offset = pointer_offset + elm_mapped;
    }
    header_->objects += elements;
  }
  /**
   * Get bulk elements, will fill the pointer with as many elements as are valid, otherwise
   * nothing.
   *
   * @param: i Location of first object to be read
   * @param: elements Number of elements to copy
   * @param: objects Pointer to array of elements
   */
  void GetBulk(std::size_t const &i, std::size_t &elements, type *objects)
  {
    file_handle_.clear();
    assert(filename_ != "");
    std::error_code error;
    // Figure out how many elements are valid to get, only get those
    if (i >= header_->objects)
    {
      return;
    }
    if (!objects)  // pointer is null
    {
      return;
    }

    assert(header_->objects >= i);
    elements = std::min(elements, std::size_t(header_->objects - i));

    size_t capacity = 0, curr_in = i, elm_mapped = 0, elm_left = elements;
    size_t pos, pointer_offset = 0;

    while (elm_left > 0)
    {
      if (!IsMapped(curr_in))
      {
        MapIndex(curr_in);
      }
      // check how many elements can get from mapped file
      capacity = (mapped_index + MAX) - curr_in;
      if (elm_left < capacity)
      {
        elm_mapped = elm_left;
        elm_left   = 0;
      }
      else
      {
        elm_mapped = capacity;
        elm_left   = elm_left - elm_mapped;
      }
      pos                 = curr_in - mapped_index_;
      type *file_offset   = (reinterpret_cast<type *>(mapped_data_.data())) + pos;
      type *object_offset = objects + pointer_offset;
      memcpy((object_offset), (file_offset), sizeof(type) * elm_mapped);
      // index will be incremented by number of written object
      curr_in += elm_mapped;
      pointer_offset = pointer_offset + elm_mapped;
    }
  }

  void SetExtraHeader(header_extra_type const &he)
  {
    assert(filename_ != "");

    header_->extra = he;
  }

  header_extra_type const &header_extra() const
  {
    return header_->extra;
  }

  uint64_t Push(type const &object)
  {

    Set(header_->objects, object);
    header_->objects = header_->objects + 1;
    return header_->objects;
  }

  void Pop()
  {
    --(header_->objects);
  }

  type Top()
  {
    assert(header_->objects > 0);
    type object;

    Get((header_->objects) - 1, object);
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
    // TODO(unknown): if something is not mapped at specified index
    // if both elements are in mapping then dont create temp map
    if (i == j)
    {
      return;
    }

    assert(filename_ != "");

    if (IsMapped(i) && IsMapped(j))
    {
      type obj_i, obj_j;
      Get(i, obj_i);
      Get(j, obj_j);
      Set(i, obj_j);
      Set(j, obj_i);
    }
    else
    {
      if (!IsMapped(i))
      {
        MapIndex(i);
      }
      size_t          n2 = (j * sizeof(type)) + header_->size();
      std::error_code error;
      mio::mmap_sink  temp_data_map = mio::make_mmap_sink(filename_, n2, sizeof(type), error);
      if (error)
      {
        throw StorageException("Could not map file");
      }
      type obj_i, obj_j;
      Get(i, obj_i);
      memcpy(&obj_j, temp_data_map.data(), sizeof(type));  // Getting value from j index
      memcpy(reinterpret_cast<type *>(temp_data_map.data()), &obj_i,
             sizeof(type));  // writing value to i index
      Set(i, obj_j);
      temp_data_map.unmap();
    }
  }

  std::size_t size() const
  {
    return header_->objects;
  }

  std::size_t empty() const
  {
    return header_->objects == 0;
  }

  /**
   * Clear the file and write an 'empty' header to the file
   */
  void Clear()
  {
    Header empty_header;
    assert(filename_ != "");
    std::fstream fin(filename_, std::ios::out | std::ios::binary);
    if (!empty_header.Write(fin))
    {
      throw StorageException("Error could not write header from clear");
    }
  }

  /**
   * Flushing writes the header to disk - there isn't necessarily any need to keep writing to disk
   * with every push etc.
   *
   * @param: lazy Whether to execute user defined callbacks
   */
  void Flush(bool const &lazy = false)
  {

    if (!lazy)
    {
      SignalBeforeFlush();
    }
    file_handle_.flush();
    std::error_code error;
    if (mapped_data_.is_mapped())
    {
      mapped_data_.sync(error);
      if (error)
      {
        throw StorageException("Could not Sync data map");
      }
    }
    if (mapped_header_.is_mapped())
    {
      error.clear();
      mapped_header_.sync(error);
      if (error)
      {
        throw StorageException("Could not Sync Header map");
      }
    }
  }

  bool is_open() const
  {
    return bool(file_handle_) && (file_handle_.is_open());
  }

private:
  /**
   * Check if file is mapped at specific index.
   *
   * @param: i The Ith object, indexed from 0
   *
   */

  bool IsMapped(std::size_t i) const
  {
    return (i >= mapped_index_ && i < (mapped_index + MAX));
  }
  //////////////////////////////
  void MapIndex(std::size_t const &i)
  {
    mapped_data_.unmap();
    mapped_index_       = i - (i % MAX);
    size_t mapped_block = std::min(i / MAX, MAX);
    size_t start        = (mapped_index * sizeof(type)) + header_->size();
    size_t pos          = start + (sizeof(type) * mapped_block);
    if (pos > GetFileLength())
    {
      ResizeFile();
    }
    std::error_code error;
    mapped_data_.map(filename_, start, sizeof(type) * MAX, error);
    if (error)
    {
      throw StorageException(" Could not map file");
    }
    // mapped_index = i;
  }
  ///////////////////////
  size_t GetFileLength()
  {
    file_handle_.clear();
    file_handle_.seekg(0, file_handle_.end);
    size_t pos = size_t(file_handle_.tellg());
    return pos;
  }
  //////////////////////
  void ResizeFile()
  {
    int64_t resizeLength = sizeof(type) * MAX;
    char    byteValue    = '\0';
    int64_t pos          = long(header_->size() + (sizeof(type) * (MAX + mapped_index_)));
    file_handle_.seekg(pos, std::ios::beg);
    std::fill_n((std::ostreambuf_iterator<char>(file_handle_)), resizeLength, byteValue);
    file_handle_.flush();
  }
  ///////////////////////
  void InitializeMapping()
  {
    Header          tempHeader;
    std::error_code error;

    mapped_header_ = mio::make_mmap_sink(filename_, 0, tempHeader.size(), error);
    if (error)
    {
      throw StorageException("Could not map Header");
    }
    header_ = reinterpret_cast<Header *>(mapped_header_.data());
    error.clear();

    uint64_t fileLength = sizeof(type) * MAX;

    mapped_data_ = mio::make_mmap_sink(filename_, tempHeader.size(), fileLength, error);
    if (error)
    {
      throw StorageException("Could not map file");
    }
    mapped_index_ = 0;
  }

  ////////////////////////////////
  event_handler_type on_file_loaded_;
  event_handler_type on_before_flush_;
  mio::mmap_sink     mapped_data_;
  mio::mmap_sink     mapped_header_;
  std::fstream       file_handle_;
  std::string        filename_ = "";
  Header *           header_;
  std::size_t        mapped_index_ = 0;
};
}  // namespace storage
}  // namespace fetch
