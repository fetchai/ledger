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
public:
  using self_type = ByteArrayBufferEx;
  using byte_array_type = byte_array::ByteArray;
  using size_counter_type = serializers::SizeCounter<self_type>;

  ByteArrayBufferEx()
  {}

  ByteArrayBufferEx(byte_array::ByteArray s)
  {
    data_ = s;
  }

  void Allocate(std::size_t const &delta)
  {
    data_.Resize(data_.size() + delta);
  }

  //TODO(pbukva) (private issue: either implementation is incorrect, or it feels like existence of this method doesn't make sense)
  void Reserve(std::size_t const delta)
  {
    data_.Reserve(data_.size() + delta);
  }

  void WriteBytes(uint8_t const *arr, std::size_t const &size)
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      data_[pos_++] = arr[i];
    }
  }

  void ReadBytes(uint8_t *arr, std::size_t const &size)
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      arr[i] = data_[pos_++];
    }
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

using ByteArrayBuffer = ByteArrayBufferEx<>;

}  // namespace serializers
}  // namespace fetch
