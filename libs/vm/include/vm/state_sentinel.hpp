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

#include <cstdint>
#include <stdexcept>
#include <string>

namespace fetch {
namespace vm {

class ReadWriteInterface
{
public:
  virtual ~ReadWriteInterface()                                                  = default;
  virtual bool read(uint8_t *dest, uint64_t dest_size, uint8_t const *const key,
                    uint64_t key_size)                                           = 0;
  virtual bool write(uint8_t const *const source, uint64_t dest_size, uint8_t const *const key,
                     uint64_t key_size)                                          = 0;
  virtual bool exists(uint8_t const *const key, uint64_t key_size, bool &exists) = 0;
};

class StateSentinel
{
public:
  StateSentinel() = default;

  bool exists(std::string const &str)
  {
    if (!read_write_interface_)
    {
      throw std::runtime_error("Failed to access state pointer in VM! Not set.");
    }

    bool ret = false;

    bool success = read_write_interface_->exists(
        reinterpret_cast<uint8_t const *const>(str.c_str()), str.size(), ret);

    if (!success)
    {
      throw std::runtime_error("Failed to access state in VM! Bad access.");
    }

    return ret;
  }

  template <typename T>
  T get(std::string const &str)
  {
    if (!read_write_interface_)
    {
      throw std::runtime_error("Failed to access state pointer in VM! Not set.");
    }

    T ret{};

    bool success = read_write_interface_->read(reinterpret_cast<uint8_t *>(&ret), sizeof(T),
                                               reinterpret_cast<uint8_t const *const>(str.c_str()),
                                               str.size());

    if (!success)
    {
      throw std::runtime_error("Failed to access state in VM! Bad access.");
    }

    return ret;
  }

  template <typename T>
  void set(std::string const &str, T item)
  {
    if (!read_write_interface_)
    {
      throw std::runtime_error("Failed to access state pointer in VM! Not set.");
    }

    bool success = read_write_interface_->write(
        reinterpret_cast<uint8_t const *const>(&item), sizeof(int),
        reinterpret_cast<uint8_t const *const>(str.c_str()), str.size());

    if (!success)
    {
      throw std::runtime_error("Failed to access state in VM!");
    }
  }

  void SetReadWriteInterface(ReadWriteInterface *ptr)
  {
    read_write_interface_ = ptr;
  }

  ReadWriteInterface *GetReadWriteInterface()
  {
    return read_write_interface_;
  }

private:
  ReadWriteInterface *read_write_interface_ = nullptr;
};

}  // namespace vm
}  // namespace fetch
