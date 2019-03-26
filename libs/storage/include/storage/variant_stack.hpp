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

//     ┌────────────────────────────────────────────────────────────────────────┐
//     │                                                                        │
//     │                                                                        ▼
//  ┌──────┐      ┌──────────────┬──────┬──────────────┬──────┬──────────────┬──────┐
//  │      │      │              │      │              │      │              │      │
//  │      │      │              │      │              │      │              │      │
//  │      │      │              │      │              │      │              │      │
//  │HEADER│......│    OBJECT    │SEPER.│    OBJECT    │SEPER.│    OBJECT    │SEPER.│
//  │      │      │              │      │              │      │              │      │
//  │      │      │              │      │              │      │              │      │
//  │      │      │              │      │              │      │              │      │
//  └──────┘      └──────────────┴──────┴──────────────┴──────┴──────────────┴──────┘
//                                  ▲                     │                     │
//                                  │                     │                     │
//                                  └─────────────────────┴─────────────────────┘

#include "core/assert.hpp"
#include "storage/storage_exception.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <string>

namespace fetch {
namespace storage {

// TODO(issue 5): Make variant stack as a circular buffer!

/**
 * Variant stack manages a stack of arbitrary objects (variant). The basic structure can be seen
 * in the header of this file. The structure is written to and read from disk.
 *
 * The structure is composed of a header and N object-separator pairs. The header points to the
 * last pair, and each separator object refers to its previous.
 *
 * The user must assign and maintain their own enums for the types of the objects in the stack, so
 * that when popping them they can be correctly cast.
 *
 */
class VariantStack
{
public:
  static constexpr char const *LOGGING_NAME = "VariantStack";

  /**
   * Separator holding information about previous object.
   */
  struct Separator
  {
    uint64_t type{0};
    uint64_t object_size{0};
    int64_t  previous{0};

    Separator() = default;
    Separator(uint64_t t, uint64_t o, int64_t p)
      : type{t}
      , object_size{o}
      , previous{p}
    {}
  };

  // This check is necessary to ensure structures are correctly packed
  static_assert(sizeof(Separator) == 24, "Header structure must be packed");

  /**
   * Header holding information about the stack
   */
  struct Header
  {
    uint64_t object_count = 0;
    int64_t  end          = 0;

    Header() = default;
    Header(uint64_t o, int64_t e)
      : object_count{o}
      , end{e}
    {}
  };

  // This check is necessary to ensure structures are correctly packed
  static_assert(sizeof(Header) == 16, "Header structure must be packed");

  ~VariantStack()
  {
    Close();
  }

  void Load(std::string const &filename, bool const &create_if_not_exists = true)
  {
    std::cerr << "Load VS: " << filename << std::endl; // DELETEME_NH

    filename_    = filename;
    file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_handle_)
    {
      if (create_if_not_exists)
      {
        std::cerr << "clearing." << std::endl; // DELETEME_NH
        Clear();
        file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
      }
      else
      {
        throw StorageException("Could not load file");
      }
    }

    ReadHeader();
  }

  void New(std::string const &filename)
  {
    std::cerr << "New VS: " << filename << std::endl; // DELETEME_NH
    filename_ = filename;
    Clear();
    file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
    assert(bool(file_handle_));
  }

  void Close()
  {
    WriteHeader();
    if (file_handle_.is_open())
    {
      file_handle_.close();
    }
  }

  enum
  {
    UNDEFINED_POSITION = int64_t(-1)
  };
  enum
  {
    HEADER_OBJECT = uint64_t(-2)
  };

  /**
   * Push a T object onto the stack, optionally specifying its type. This will simply reinterpret
   * cast the object as a char * for the write. Then write the separator information and update the
   * header
   *
   * @param: object The object to write
   * @param: type Optionally specify a type to differentiate objects
   *
   */
  template <typename T>
  void Push(T const &object, uint64_t const &type = uint64_t(-1))
  {
    assert(bool(file_handle_));
    file_handle_.seekg(header_.end, file_handle_.beg);
    Separator separator = {type, sizeof(T), header_.end};

    file_handle_.write(reinterpret_cast<char const *>(&object), sizeof(T));
    file_handle_.write(reinterpret_cast<char const *>(&separator), sizeof(Separator));
    header_.end += sizeof(T) + sizeof(Separator);
    ++header_.object_count;
  }

  /**
   * Pop the topmost object off the stack. Unsafe if there are no objects on the stack.
   */
  void Pop()
  {
    assert(header_.object_count != 0);
    file_handle_.seekg(header_.end - int64_t(sizeof(Separator)), file_handle_.beg);
    Separator separator;

    file_handle_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));

    header_.end = separator.previous;
    --header_.object_count;
  }

  /**
   * Fill the provided object with the object provided
   *
   * @param: object The reference to fill
   *
   * @return: The type of the object as specified during its initial write onto the stack
   */
  template <typename T>
  uint64_t Top(T &object)
  {
    assert(bool(file_handle_));

    file_handle_.seekg(header_.end - int64_t(sizeof(Separator)), file_handle_.beg);
    Separator separator;

    file_handle_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));
    int64_t offset = int64_t(sizeof(Separator) + separator.object_size);

    if (separator.object_size != sizeof(T))
    {
      std::ostringstream ret;
      ret << "Size mismatch in variantstack when accessing Top. ";
      ret << "Expected: " << separator.object_size << ". ";
      ret << "Got: " << sizeof(T) << ". ";

      throw StorageException(ret.str());
    }

    file_handle_.seekg(header_.end - offset, file_handle_.beg);
    file_handle_.read(reinterpret_cast<char *>(&object), sizeof(T));
    return separator.type;
  }

  /**
   * Return the type of the object on the top of the stack
   *
   * @return: The type of the object
   */
  uint64_t Type()
  {
    file_handle_.seekg(header_.end - int64_t(sizeof(Separator)), file_handle_.beg);
    Separator separator;

    file_handle_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));

    return separator.type;
  }

  /**
   * Reset the state of the file handle to starting conditions. This consists of a header and a
   * separator (it is convenient to have a starting invalid separator).
   *
   */
  void Clear()
  {
    std::cerr << "*** cleared the variant stack!" << std::endl; // DELETEME_NH

    assert(filename_ != "");
    std::fstream fin(filename_, std::ios::out | std::ios::binary);
    fin.seekg(0, fin.beg);

    Separator separator = {HEADER_OBJECT, 0, UNDEFINED_POSITION};

    header_     = Header();
    header_.end = sizeof(Header) + sizeof(Separator);

    fin.write(reinterpret_cast<char const *>(&header_), sizeof(Header));
    fin.write(reinterpret_cast<char const *>(&separator), sizeof(Separator));

    fin.close();
  }

  bool empty() const
  {
    return header_.object_count == 0;
  }

  std::size_t size() const
  {
    return std::size_t(header_.object_count);
  }

  void Flush(bool lazy = false)
  {
    WriteHeader();
  }

protected:
  void ReadHeader()
  {
    file_handle_.seekg(0, file_handle_.beg);
    file_handle_.read(reinterpret_cast<char *>(&header_), sizeof(Header));

    std::cerr << "*** After reading header, size is : " << size() << " and file size is : " << file_handle_.tellg() << std::endl; // DELETEME_NH
  }

  void WriteHeader()
  {
    file_handle_.seekg(0, file_handle_.beg);
    file_handle_.write(reinterpret_cast<char const *>(&header_), sizeof(Header));
  }

private:
  std::fstream file_handle_;
  std::string  filename_ = "";
  Header       header_;
};
}  // namespace storage
}  // namespace fetch
