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
#include <string>

#include "core/assert.hpp"
#include "storage/fetch_mmap.hpp"
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
 * The MMapRandomAccessStack maintains a stack of type T, writing to disk. Since elements on the
 * stack are uniform size, they can be easily addressed using simple arithmetic.
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

  MMapRandomAccessStack()
  {
    throw std::runtime_error("This class hasn't been fully tested for production code");
  }

  // Enable constructor for unit tests
  MMapRandomAccessStack(const char *is_testing)
  {
    if (!(std::string(is_testing).compare(std::string("test")) == 0))
    {
      throw std::runtime_error("This class hasn't been fully tested for production code");
    }
  }

  ~MMapRandomAccessStack()
  {
    if (file_handle_.is_open())
    {
      Close(false);
    }
  }

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

  /**
   * Closes the stack and flush contents to file
   *
   * @lazy: whether to content should be flushed to file or does a lazy flushing.
   */
  void Close(bool const &lazy = false)
  {
    if (!lazy)
    {
      Flush(lazy);
    }
    mapped_data_.unmap();
    mapped_header_.unmap();
    file_handle_.close();
  }
  /**
   *  Load file from disk and if files does not exist already then file will be created.
   *
   * @param: filename Name of the file to be loaded
   * @param: create_if_not_exist If file with this name does not exist already then create it.
   *
   */
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
  /**
   *  Create a new file on disk
   *
   * @param: filename Name of the file to be opened
   *
   */
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
    InitializeMapping();
    SignalFileLoaded();
  }
  /**
   * Get object from the stack at index i, not safe when i > objects.
   *
   * @param: i The Ith object, indexed from 0
   * @param: object The object to copy from the stack
   *
   */
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
   * update it if necessary.
   *
   * @param: i Location of first object to be written
   * @param: elements Number of elements to copy
   * @param: objects Pointer to array of elements
   *
   */
  void SetBulk(std::size_t const &i, std::size_t elements, type *objects)
  {
    assert(filename_ != "");
    if ((i + elements) > header_->objects)
    {
      ResizeFile(i, elements);  // Resize file if needed
    }
    std::size_t curr_in = i, elm_mapped = 0, src_index = 0, elm_left = elements;
    for (; elm_left > 0; curr_in += elm_mapped, src_index += elm_mapped, elm_left -= elm_mapped)
    {
      if (!IsMapped(curr_in))
      {
        MapIndex(curr_in);
      }
      elm_mapped              = std::min((mapped_index_ + MAX) - curr_in, elm_left);
      std::size_t block_index = curr_in - mapped_index_;
      type *      file_offset = (reinterpret_cast<type *>(mapped_data_.data()));
      memcpy(&(file_offset[block_index]), &objects[src_index], sizeof(type) * elm_mapped);
    }
    std::size_t written_elements = (i + elements) - header_->objects;
    if (written_elements > 0)
    {
      header_->objects += written_elements;
    }
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
    assert(objects != nullptr);

    // Figure out how many elements are valid to get, only get those
    elements = std::min(elements, std::size_t(header_->objects - i));

    std::size_t curr_in = i, elm_mapped = 0, src_index = 0, elm_left = elements;
    for (; elm_left > 0; curr_in += elm_mapped, src_index += elm_mapped, elm_left -= elm_mapped)
    {
      if (!IsMapped(curr_in))
      {
        MapIndex(curr_in);
      }
      elm_mapped              = std::min((mapped_index_ + MAX) - curr_in, elm_left);
      std::size_t block_index = curr_in - mapped_index_;
      type *      file_offset = (reinterpret_cast<type *>(mapped_data_.data()));
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
      std::size_t     j_offset = (j * sizeof(type)) + header_->size();
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
    if (!mapped_data_.is_mapped())
    {  // Initially nothing is mapped
      return false;
    }
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
    mapped_index_              = i - (i % MAX);
    std::size_t mapping_start  = (mapped_index_ * sizeof(type)) + header_->size();
    std::size_t mapping_length = mapping_start + (sizeof(type) * MAX);
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

  std::size_t GetFileLength()
  {
    file_handle_.clear();
    file_handle_.seekg(0, file_handle_.end);
    std::size_t pos = std::size_t(file_handle_.tellg());
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
  /*
   * memory mapping is not possible on empty file so we need to
   *
   * extend the file to write any object at the end of file.
   *
   * @index: location where data will be written
   * @elements: elements to be written. File size will be extend if elements exceeds the existing
   * file size.
   */
  void ResizeFile(size_t index, std::size_t elements)
  {
    std::size_t obj_capacity, expected_obj_count, adjusted_obj_count, block_count, total_length;

    obj_capacity       = (GetFileLength() - header_->size()) / sizeof(type);
    expected_obj_count = index + elements;
    adjusted_obj_count = expected_obj_count - obj_capacity;

    // check if extended objects are multiple of MAX
    block_count =
        adjusted_obj_count % MAX == 0 ? adjusted_obj_count / MAX : adjusted_obj_count / MAX + 1;
    adjusted_obj_count = block_count * MAX;

    total_length = GetFileLength() + adjusted_obj_count * sizeof(type);
    try
    {
      file_handle_.seekg((long)total_length, std::ios::beg);
      std::fill_n((std::ostreambuf_iterator<char>(file_handle_)), adjusted_obj_count * sizeof(type),
                  '\0');
      file_handle_.flush();
    }
    catch (storage::StorageException const &e)
    {
      std::cerr << "error: " << e.what() << std::endl;
    }
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
