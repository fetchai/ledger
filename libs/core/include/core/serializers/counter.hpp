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

#include "core/serializers/stl_types.hpp"

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/type_register.hpp"
#include <type_traits>

namespace fetch {
namespace serializers {

template <typename S>
class SizeCounter
{
public:
  using self_type = SizeCounter;

  void Allocate(std::size_t const &val)
  {
    size_ += val;
  }

  //TODO(pbukva) (private issue: this is exactly what ByteArrayBuffer does even if it does not make much sense)
  void Reserve(std::size_t const &val)
  {
    size_ += val;
  }

  void WriteBytes(uint8_t const *, std::size_t const &size)
  {
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
  self_type &Pack(T const *val)
  {
    return this->operator<<(val);
  }

  template <typename T>
  self_type &Pack(T const &val)
  {
    return this->operator<<(val);
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
    return size_;
  }

  int64_t bytes_left() const
  {
    return static_cast<int64_t>(size_ - pos_);
  }

  template<typename ...ARGS>
  self_type & Append(ARGS const&... args)
  {
    AppendInternal(args...);
    return *this;
  }

private:
  template<typename T, typename ...ARGS>
  void AppendInternal(T const &arg, ARGS const&... args)
  {
    std::cout << "SizeCount.AppendInternal(" << typeid(arg).name() << ")" << std::endl;
    *this << arg;
    AppendInternal(args...);
  }

  void AppendInternal()
  {
  }

  std::size_t size_ = 0;
  std::size_t pos_ = 0;
};

}  // namespace serializers
}  // namespace fetch
