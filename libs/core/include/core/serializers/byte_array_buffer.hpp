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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/counter.hpp"

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace serializers {

template <typename X = void>
class ByteArrayBufferEx
{
  static char const *const LOGGING_NAME;

public:
  using self_type         = ByteArrayBufferEx;
  using byte_array_type   = byte_array::ByteArray;
  using size_counter_type = serializers::SizeCounter<self_type>;

  ByteArrayBufferEx()                         = default;
  ByteArrayBufferEx(ByteArrayBufferEx &&from) = default;
  ByteArrayBufferEx &operator=(ByteArrayBufferEx &&from) = default;

  /**
   * @brief Contructting from MUTABLE ByteArray.
   *
   * DEEP copy is made here due to safety reasons to avoid later
   * misshaps & missunderstrandings related to what hapens with reserved
   * memory of mutable @ref s instance passed in by caller of this
   * constructor once this class starts to modify content of underlaying
   * internal @ref data_ ByteArray and then resize/reserve it.
   *
   * @param s Input mutable instance of ByteArray to copy content from (by
   *          value as explained above)
   */
  ByteArrayBufferEx(byte_array::ByteArray const &s)
    : data_{s.Copy()}
  {}

  ByteArrayBufferEx(ByteArrayBufferEx const &from)
    : data_{from.data_.Copy()}
    , pos_{from.pos_}
    , size_counter_{from.size_counter_}
  {}

  ByteArrayBufferEx(std::size_t capacity)
    : data_(capacity)
    , pos_{0}
    , size_counter_{capacity}
  {}

  ByteArrayBufferEx &operator=(ByteArrayBufferEx const &from)
  {
    *this = ByteArrayBufferEx{from};
    return *this;
  }

  void Allocate(std::size_t delta)
  {
    Resize(delta, ResizeParadigm::RELATIVE);
  }

  void Resize(std::size_t size, ResizeParadigm const &resize_paradigm = ResizeParadigm::RELATIVE,
              bool const zero_reserved_space = true)
  {
    data_.Resize(size, resize_paradigm, zero_reserved_space);

    switch (resize_paradigm)
    {
    case ResizeParadigm::RELATIVE:
      break;

    case ResizeParadigm::ABSOLUTE:
      if (pos_ > size)
      {
        seek(size);
      }
      break;
    };
  }

  void Reserve(std::size_t size, ResizeParadigm const &resize_paradigm = ResizeParadigm::RELATIVE,
               bool const zero_reserved_space = true)
  {
    data_.Reserve(size, resize_paradigm, zero_reserved_space);
  }

  void WriteBytes(uint8_t const *arr, std::size_t size)
  {
    data_.WriteBytes(arr, size, pos_);
    pos_ += size;
  }

  void ReadBytes(uint8_t *arr, std::size_t size)
  {
    data_.ReadBytes(arr, size, pos_);
    pos_ += size;
  }

  void ReadByteArray(byte_array::ConstByteArray &b, std::size_t size)
  {
    b = data_.SubArray(pos_, size);
    pos_ += size;
  }

  void SkipBytes(std::size_t size)
  {
    pos_ += size;
  }

  template <typename T>
  self_type &operator<<(T const *val)
  {
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  self_type &operator<<(T const &val)
  {
    Serialize(*this, val);
    return *this;
  }

  template <typename T>
  self_type &operator>>(T &val)
  {
    Deserialize(*this, val);
    return *this;
  }

  template <typename T>
  self_type &Pack(T const *val)
  {
    return this->operator<<(val);
  }

  template <typename T>
  self_type &Pack(T const &val)
  {
    return this->operator<<(val);
  }

  template <typename T>
  self_type &Unpack(T &val)
  {
    return this->operator>>(val);
  }

  // FIXME: Incorrect naming
  void seek(std::size_t p)
  {
    pos_ = p;
  }
  std::size_t tell() const
  {
    return pos_;
  }

  std::size_t size() const
  {
    return data_.size();
  }

  std::size_t capacity() const
  {
    return data_.capacity();
  }

  int64_t bytes_left() const
  {
    return int64_t(data_.size()) - int64_t(pos_);
  }
  byte_array::ByteArray const &data() const
  {
    return data_;
  }

  template <typename... ARGS>
  self_type &Append(ARGS const &... args)
  {
    auto size_count_guard = sizeCounterGuardFactory(size_counter_);
    if (size_count_guard.is_unreserved())
    {
      size_counter_.Allocate(size());
      size_counter_.seek(tell());

      size_counter_.Append(args...);
      if (size() < size_counter_.size())
      {
        Reserve(size_counter_.size() - size());
      }
    }

    AppendInternal(args...);
    return *this;
  }

private:
  template <typename T, typename... ARGS>
  void AppendInternal(T const &arg, ARGS const &... args)
  {
    *this << arg;
    AppendInternal(args...);
  }

  void AppendInternal()
  {}

  byte_array_type   data_;
  std::size_t       pos_ = 0;
  size_counter_type size_counter_;
};

template <typename T>
char const *const ByteArrayBufferEx<T>::LOGGING_NAME = "ByteArrayBuffer<...>";

using ByteArrayBuffer = ByteArrayBufferEx<>;

}  // namespace serializers
}  // namespace fetch
