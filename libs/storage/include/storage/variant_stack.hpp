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
#include <cassert>
#include <cstring>
#include <fstream>
#include <string>

namespace fetch {
namespace storage {

// TODO(issue 5): Make variant stack as a circular buffer!
// TODO: (issue 39) : add exceptions (HUT)
// TODO: (issue 40) : use serializer for objects (HUT)

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
  /**
   * Seperator holding information about previous object.
   */
  struct Separator
  {
    uint64_t type;
    uint64_t object_size;
    int64_t  previous;
    Separator() { memset(this, 0, sizeof(decltype(*this))); }
    Separator(uint64_t const &t, uint64_t const &o, int64_t const &p)
    {
      memset(this, 0, sizeof(decltype(*this)));
      type        = t;
      object_size = o;
      previous    = p;
    }
  };

  /**
   * Header holding information about the stack
   */
  struct Header
  {
    uint64_t object_count;
    int64_t  end;

    Header(uint64_t const &o, int64_t const &e)
    {
      memset(this, 0, sizeof(decltype(*this)));
      object_count = o;
      end          = e;
    }

    Header()
    {
      memset(this, 0, sizeof(decltype(*this)));
      end = sizeof(decltype(*this)) + sizeof(Separator);
    }
  };

  ~VariantStack() { Close(); }

  void Load(std::string const &filename, bool const &create_if_not_exists = true)
  {
    filename_ = filename;
    file_     = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_)
    {
      if (create_if_not_exists)
      {

        Clear();
        file_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
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
    filename_ = filename;
    Clear();
    file_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
    assert(bool(file_));
  }

  void Close()
  {
    WriteHeader();
    if (file_.is_open())
    {
      file_.close();
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
    assert(bool(file_));
    file_.seekg(header_.end, file_.beg);
    Separator separator = {type, sizeof(T), header_.end};

    file_.write(reinterpret_cast<char const *>(&object), sizeof(T));
    file_.write(reinterpret_cast<char const *>(&separator), sizeof(Separator));
    header_.end += sizeof(T) + sizeof(Separator);
    ++header_.object_count;
    //    WriteHeader();
  }

  /**
   * Pop the topmost object off the stack. Unsafe if there are no objects on the stack.
   */
  void Pop()
  {
    assert(header_.object_count != 0);
    file_.seekg(header_.end - int64_t(sizeof(Separator)), file_.beg);
    Separator separator;

    file_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));

    header_.end = separator.previous;
    --header_.object_count;
    //    WriteHeader();
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
    assert(bool(file_));

    file_.seekg(header_.end - int64_t(sizeof(Separator)), file_.beg);
    Separator separator;

    file_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));
    int64_t offset = int64_t(sizeof(Separator) + separator.object_size);

    if (separator.object_size != sizeof(T))
    {
      std::cout << offset << " " << separator.object_size << " " << sizeof(T) << std::endl;

      throw StorageException("TODO: Throw error, size mismatch in variantstack");
    }

    file_.seekg(header_.end - offset, file_.beg);
    file_.read(reinterpret_cast<char *>(&object), sizeof(T));
    return separator.type;
  }

  /**
   * Return the type of the object on the top of the stack
   *
   * @return: The type of the object
   */
  uint64_t Type()
  {
    file_.seekg(header_.end - int64_t(sizeof(Separator)), file_.beg);
    Separator separator;

    file_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));

    return separator.type;
  }

  /**
   * Reset the state of the file handle to starting conditions. This consists of a header and a
   * separator (it is convenient to have a starting invalid separator).
   *
   */
  void Clear()
  {
    assert(filename_ != "");
    std::fstream fin(filename_, std::ios::out | std::ios::binary);
    fin.seekg(0, fin.beg);

    Separator separator = {HEADER_OBJECT, 0, UNDEFINED_POSITION};

    header_ = Header();
    fin.write(reinterpret_cast<char const *>(&header_), sizeof(Header));
    fin.write(reinterpret_cast<char const *>(&separator), sizeof(Separator));

    fin.close();
  }

  bool    empty() const { return header_.object_count == 0; }
  int64_t size() const { return int64_t(header_.object_count); }

protected:
  void ReadHeader()
  {
    file_.seekg(0, file_.beg);
    file_.read(reinterpret_cast<char *>(&header_), sizeof(Header));
  }

  void WriteHeader()
  {
    file_.seekg(0, file_.beg);
    file_.write(reinterpret_cast<char const *>(&header_), sizeof(Header));
  }

private:
  std::fstream file_;
  std::string  filename_ = "";
  Header       header_;
};
}  // namespace storage
}  // namespace fetch
