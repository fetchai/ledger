#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "core/serializers/stl_types.hpp"

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/type_register.hpp"
#include <type_traits>

namespace fetch {
namespace serializers {

class TypedByteArrayBuffer;

template <typename S>
class SizeCounter
{
public:
  void Allocate(std::size_t const &val) { size_ += val; }

  void Reserve(std::size_t const &val) {}

  void WriteBytes(uint8_t const *arr, std::size_t const &size) {}
  void ReadBytes(uint8_t const *arr, std::size_t const &size) {}

  void SkipBytes(std::size_t const &size) {}

  template <typename T>
  SizeCounter &operator<<(T const &val)
  {
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  SizeCounter &Pack(T const &val)
  {
    return this->operator<<(val);
  }

  void        Seek(std::size_t const &p) {}
  std::size_t Tell() const { return 0; }
  int64_t     bytes_left() const { return 0; }
  std::size_t size() const { return size_; }

private:
  std::size_t size_ = 0;
};

template <>
class SizeCounter<TypedByteArrayBuffer>
{
public:
  void Allocate(std::size_t const &val) { size_ += val; }

  void Reserve(std::size_t const &val) {}

  void WriteBytes(uint8_t const *arr, std::size_t const &size) {}
  void ReadBytes(uint8_t const *arr, std::size_t const &size) {}

  void SkipBytes(std::size_t const &size) {}

  template <typename T>
  SizeCounter &operator<<(T const &val)
  {
    Serialize(*this, TypeRegister<void>::value_type(TypeRegister<T>::value));
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  SizeCounter &Pack(T const &val)
  {
    return this->operator<<(val);
  }

  void        Seek(std::size_t const &p) {}
  std::size_t Tell() const { return 0; }

  int64_t bytes_left() const { return 0; }

  std::size_t size() const { return size_; }

private:
  std::size_t size_ = 0;
};
}  // namespace serializers
}  // namespace fetch
