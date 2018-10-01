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
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/counter.hpp"

#include "core/logger.hpp"
#include <type_traits>

namespace fetch {
namespace serializers {


template<typename X=void>
class ByteArrayBufferEx
{
  static char const * const LOGGING_NAME;

public:
  using self_type = ByteArrayBufferEx;
  using byte_array_type = byte_array::ByteArray;
  using size_counter_type = serializers::SizeCounter<self_type>;

  ByteArrayBufferEx() = default;
  ByteArrayBufferEx(ByteArrayBufferEx &&from) = default;
  ByteArrayBufferEx& operator = (ByteArrayBufferEx &&from) = default;

  //* The only safe way is to make a deep copy here to avoid later
  //* missunderstanding what hapens with reserved memory of original
  //* ByteArray input instance passed in by caller of this constructor.
  //*
  //* *IF* stream does *NOT* need to re-allocate while using this
  //* ByteArrayBuffer instance, then original memory
  //* allocated by input ByteArray instance parameter will be used,
  //* what means that caller who passed ByteArray will see *ALL* the
  //* changes made in this ByteArryBuffer.
  //*
  //* *HOWEVER, IF* this ByteArryBuffer instance will need to
  //* re-allocate, then caller who passed in original input ByteArray
  //* instance will see only partial changes mad until re-allocate
  //* has been requested from this ByteArrayBuffer instance.
  ByteArrayBufferEx(byte_array::ByteArray s)
    : data_{s.Copy()}
  {
  }

  ByteArrayBufferEx(ByteArrayBufferEx const &from)
    : data_{from.data_.Copy()}
    , pos_{from.pos_}
    , size_counter_{from.size_counter_}
  {
  }

  ByteArrayBufferEx & operator = (ByteArrayBufferEx const &from)
  {
    *this = ByteArrayBufferEx{ from };
    //data_ = from.data_.Copy();
    //pos_ = from.pos_;
    //size_counter_ = std::move(from.size_counter_);
    return *this;
  }

  void Allocate(std::size_t const &delta)
  {
    Resize(delta, ResizeParadigm::RELATIVE);
  }

  void Resize(std::size_t const &size, ResizeParadigm const& resize_paradigm = ResizeParadigm::RELATIVE, bool const zero_reserved_space=true)
  {
    data_.Resize(size, resize_paradigm, zero_reserved_space);

    switch(resize_paradigm)
    {
      case ResizeParadigm::RELATIVE:
        break;

      case ResizeParadigm::ABSOLUTE:
        if(pos_ > size)
        {
          Seek(size);
        }
        break;
    };
  }

  void Reserve(std::size_t const &size, ResizeParadigm const& resize_paradigm = ResizeParadigm::RELATIVE, bool const zero_reserved_space=true)
  {
    data_.Reserve(size, resize_paradigm, zero_reserved_space);
  }

  void WriteBytes(uint8_t const *arr, std::size_t const &size)
  {
    data_.WriteBytes(arr, size, pos_);
    pos_ += size;
  }

  void ReadBytes(uint8_t *arr, std::size_t const &size)
  {
    data_.ReadBytes(arr, size, pos_);
    pos_ += size;
  }

  void ReadByteArray(byte_array::ConstByteArray &b, std::size_t const &size)
  {
    b = data_.SubArray(pos_, size);
    pos_ += size;
  }

  void SkipBytes(std::size_t const &size)
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
  void Seek(std::size_t const &p)
  {
    pos_ = p;
  }
  std::size_t Tell() const
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

  template<typename ...ARGS>
  self_type & Append(ARGS const&... args)
  {
    auto size_count_guard = sizeCounterGuardFactory(size_counter_);
    if (size_count_guard.is_unreserved())
    {
      size_counter_.Allocate(size());
      size_counter_.Seek(Tell());

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
  template<typename T, typename ...ARGS>
  void AppendInternal(T const &arg, ARGS const&... args)
  {
    *this << arg;
    AppendInternal(args...);
  }

  void AppendInternal()
  {
  }

  byte_array_type data_;
  std::size_t pos_ = 0;
  size_counter_type size_counter_;
 };

template<typename T>
char const * const ByteArrayBufferEx<T>::LOGGING_NAME = "ByteArrayBuffer<...>";

using ByteArrayBuffer = ByteArrayBufferEx<>;


}  // namespace serializers
}  // namespace fetch
