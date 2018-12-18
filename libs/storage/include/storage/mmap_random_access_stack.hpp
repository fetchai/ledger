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
 *
 * MAX is the count of map-able Objects at one time
 */

template <typename T, typename D = uint64_t, unsigned long MAX = 256>
class MMapRandomAccessStack
{
private:
  static constexpr char const *LOGGING_NAME = "MMapRandomAccessStack";

/**
 * Header holding information for the structure. Magic is used to determine the endianness of the
 * platform, extra allows the user to write metadata for the structure. This is used for example
 * in key value store to store the head of the trie
 */
#pragma pack(push, 1)  // Packing used to avoid byte padding while ready/writing pointer from file
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

  ~MMapRandomAccessStack()
  {
    if (file_handle_.is_open())
    {
      Close(false);
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
      SignalFileLoaded();
    }
  }

  //////////////////////////////////
  void New(std::string const &filename)
  {
    filename_ = filename;
    file_handle_ =
        std::fstream(filename_, std::ios::in | std::ios::out | std::ios::app | std::ios::binary);
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

    std::size_t index          = i - mapped_index_;
    type *      mapped_objects = reinterpret_cast<type *>(mapped_data_.data());
    memcpy((&object), &(mapped_objects[index]), sizeof(object));
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

    std::size_t index          = i - mapped_index_;
    type *      mapped_objects = reinterpret_cast<type *>(mapped_data_.data());
    memcpy(&(mapped_objects[index]), &object, sizeof(type));
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
    // TODO(unknown): EXCEPTION HANDLING: While writing chunks if fails in middle and file state is
    // changed
    assert(filename_ != "");
    size_t curr_in = i, elm_mapped = 0, src_index = 0;
    for (; elements > 0; curr_in += elm_mapped, src_index += elm_mapped, elements -= elm_mapped)
    {
      if (!IsMapped(curr_in))
      {
        MapIndex(curr_in);
      }
      elm_mapped         = std::min((mapped_index_ + MAX) - curr_in, elements);
      size_t block_index = curr_in - mapped_index_;
      type * file_offset = (reinterpret_cast<type *>(mapped_data_.data()));
      memcpy(&(file_offset[block_index]), &objects[src_index], sizeof(type) * elm_mapped);
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
    assert(filename_ != "");
    assert(header_->objects > i);
    assert(objects != NULL);  // check null or NULL

    // Figure out how many elements are valid to get, only get those
    elements = std::min(elements, std::size_t(header_->objects - i));

    size_t curr_in = i, elm_mapped = 0, src_index = 0;
    for (; elements > 0; curr_in += elm_mapped, src_index += elm_mapped, elements -= elm_mapped)
    {
      if (!IsMapped(curr_in))
      {
        MapIndex(curr_in);
      }
      elm_mapped         = std::min((mapped_index_ + MAX) - curr_in, elements);
      size_t block_index = curr_in - mapped_index_;
      type * file_offset = (reinterpret_cast<type *>(mapped_data_.data()));
      memcpy(&(objects[src_index]), &(file_offset[block_index]), sizeof(type) * elm_mapped);
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
    header_->objects++;
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
    // if both elements are in mapping then dont create temp map
    if (i == j)
    {
      return;
    }

    assert(filename_ != "");

    // If both indexes lies in already mapped area
    if (IsMapped(i) && IsMapped(j))
    {
      type obj_i, obj_j;
      Get(i, obj_i);
      Get(j, obj_j);
      Set(i, obj_j);
      Set(j, obj_i);
    }
    else  // indexes are distant and need to create a temporary mapping for one index
    {
      if (!IsMapped(i))
      {
        MapIndex(i);
      }
      size_t          j_offset = (j * sizeof(type)) + header_->size();
      std::error_code error;
      mio::mmap_sink  j_object_map = mio::make_mmap_sink(filename_, j_offset, sizeof(type), error);
      if (error)
      {
        throw StorageException("Could not map file");
      }
      type obj_i, obj_j;
      Get(i, obj_i);
      memcpy(&obj_j, j_object_map.data(), sizeof(type));  // Getting value from j index
      // setting value to j index
      memcpy(reinterpret_cast<type *>(j_object_map.data()), &obj_i, sizeof(type));
      Set(i, obj_j);
      j_object_map.unmap();
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
    if (mapped_data_.is_mapped())
    {
      std::error_code error;
      mapped_data_.sync(error);
      if (error)
      {
        throw StorageException("Could not Sync data map");
      }
    }
    if (mapped_header_.is_mapped())
    {
      std::error_code error;
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
    return (i >= mapped_index_ && i < (mapped_index_ + MAX));
  }
  /**
   * Map the file at specified index.
   *
   * If i's offset is greater then file-length then file will be resized.
   */
  void MapIndex(std::size_t const &i)
  {
    mapped_data_.unmap();
    mapped_index_         = i - (i % MAX);
    size_t mapping_start  = (mapped_index_ * sizeof(type)) + header_->size();
    size_t mapping_length = mapping_start + (sizeof(type) * MAX);
    if (mapping_length > GetFileLength())
    {
      ResizeFile();
    }
    std::error_code error;
    mapped_data_.map(filename_, mapping_start, sizeof(type) * MAX, error);
    if (error)
    {
      throw StorageException(" Could not map file");
    }
  }

  size_t GetFileLength()
  {
    file_handle_.clear();
    file_handle_.seekg(0, file_handle_.end);
    size_t pos = size_t(file_handle_.tellg());
    return pos;
  }
  /*
   * memory mapping is not possible on empty file so we need to
   *
   * extend the file to write any object at the end of file
   */
  void ResizeFile()
  {
    int64_t resize_length = sizeof(type) * MAX;
    int64_t total_length  = int64_t(header_->size() + (sizeof(type) * (MAX + mapped_index_)));
    file_handle_.seekg(total_length, std::ios::beg);
    std::fill_n((std::ostreambuf_iterator<char>(file_handle_)), resize_length, '\0');
    file_handle_.flush();
  }

  void InitializeMapping()
  {
    std::error_code error;
    mapped_header_ = mio::make_mmap_sink(filename_, 0, sizeof(Header), error);
    if (error)
    {
      throw StorageException("Could not map Header");
    }
    header_ = reinterpret_cast<Header *>(mapped_header_.data());

    error.clear();
    uint64_t fileLength = sizeof(type) * MAX;
    mapped_data_        = mio::make_mmap_sink(filename_, sizeof(Header), fileLength, error);
    if (error)
    {
      throw StorageException("Could not map file");
    }
    mapped_index_ = 0;
  }

  event_handler_type on_file_loaded_;
  event_handler_type on_before_flush_;
  mio::mmap_sink     mapped_data_;    // This map handles read/write objects from/to file
  mio::mmap_sink     mapped_header_;  // This map handles header part in the file
  std::fstream       file_handle_;
  std::string        filename_ = "";
  Header *           header_;
  std::size_t        mapped_index_ = 0;  // It holds the mapped index value
};
}  // namespace storage
}  // namespace fetch
