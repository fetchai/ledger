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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/serializers/exception.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/type_register.hpp"
#include <type_traits>

namespace fetch {
namespace serializers {

class TypedByteArrayBuffer
{
public:
  using byte_array_type = byte_array::ByteArray;
  TypedByteArrayBuffer() { detailed_assert(size() == 0); }
  TypedByteArrayBuffer(byte_array_type s) { data_ = s; }

  void Allocate(std::size_t const &val) { data_.Resize(data_.size() + val); }

  void Reserve(std::size_t const &val) { data_.Reserve(data_.size() + val); }

  void WriteBytes(uint8_t const *arr, std::size_t const &size)
  {
    for (std::size_t i = 0; i < size; ++i) data_[pos_++] = arr[i];
  }

  void ReadBytes(uint8_t *arr, std::size_t const &size)
  {
    if (int64_t(size) > bytes_left())
    {
      throw std::runtime_error(
          "Typed serializer error: Not enough bytes - TODO, make error "
          "serializable");
    }

    for (std::size_t i = 0; i < size; ++i) arr[i] = data_[pos_++];
  }

  void ReadByteArray(byte_array::ConstByteArray &b, std::size_t const &size)
  {
    if (int64_t(size) > bytes_left())
    {
      throw std::runtime_error(
          "Typed serializer error 2: Not enough bytes - TODO, make error "
          "serializable");
    }

    b = data_.SubArray(pos_, size);
    pos_ += size;
  }

  void SkipBytes(std::size_t const &size) { pos_ += size; }

  template <typename T>
  TypedByteArrayBuffer &operator<<(T const *val)
  {
    Serialize(*this, TypeRegister<void>::value_type(TypeRegister<T const *>::value));
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  TypedByteArrayBuffer &operator<<(T const &val)
  {
    Serialize(*this, TypeRegister<void>::value_type(TypeRegister<T>::value));
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  TypedByteArrayBuffer &operator>>(T &val)
  {
    TypeRegister<void>::value_type type;
    Deserialize(*this, type);
    if (TypeRegister<T>::value != type)
    {
      fetch::logger.Debug("Serializer at position ", pos_, " out of ", data_.size());
      fetch::logger.Error(byte_array_type("Expected type '") + TypeRegister<T>::name() +
                          byte_array_type("' differs from deserialized type '") +
                          ErrorCodeToMessage(type) + byte_array_type("'"));

      throw SerializableException(error::TYPE_ERROR,
                                  byte_array_type("Expected type '") + TypeRegister<T>::name() +
                                      byte_array_type("' differs from deserialized type '") +
                                      ErrorCodeToMessage(type) + byte_array_type("'"));
    }
    Deserialize(*this, val);
    return *this;
  }

  template <typename T>
  TypedByteArrayBuffer &Pack(T const *val)
  {
    return this->operator<<(val);
  }

  template <typename T>
  TypedByteArrayBuffer &Pack(T const &val)
  {
    return this->operator<<(val);
  }

  template <typename T>
  TypedByteArrayBuffer &Unpack(T &val)
  {
    return this->operator>>(val);
  }

  void        Seek(std::size_t const &p) { pos_ = p; }
  std::size_t Tell() const { return pos_; }

  std::size_t            size() const { return data_.size(); }
  int64_t                bytes_left() const { return int64_t(data_.size()) - int64_t(pos_); }
  byte_array_type const &data() const { return data_; }

private:
  byte_array_type data_;
  std::size_t     pos_ = 0;
};
}  // namespace serializers
}  // namespace fetch
