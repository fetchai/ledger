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

#include "core/common.hpp"
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

  void Allocate(std::size_t const &delta)
  {
    Resize(delta, eResizeParadigm::relative);
  }

  void Resize(std::size_t const &size, eResizeParadigm const& resize_paradigm = eResizeParadigm::relative, bool const zero_reserved_space=true)
  {
    (void)zero_reserved_space;

    Reserve(size, resize_paradigm);

    switch(resize_paradigm)
    {
      case eResizeParadigm::relative:
        size_ += size;
        break;

      case eResizeParadigm::absolute:
        size_ = size;
        if(pos_ > size)
        {
          Seek(size_);
        }
        break;
    };
  }

  void Reserve(std::size_t const &size, eResizeParadigm const& resize_paradigm = eResizeParadigm::relative, bool const zero_reserved_space=true)
  {
    (void)zero_reserved_space;

    switch(resize_paradigm)
    {
      case eResizeParadigm::relative:
        reserved_size_ += size;
        break;

      case eResizeParadigm::absolute:
        if (reserved_size_ < size)
        {
          reserved_size_ = size;
        }
        break;
    };
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

  std::size_t capacity() const
  {
    return reserved_size_;
  }

  int64_t bytes_left() const
  {
    return int64_t(size_) - int64_t(pos_);
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
    *this << arg;
    AppendInternal(args...);
  }

  void AppendInternal()
  {
  }

  std::size_t size_ = 0;
  std::size_t pos_ = 0;
  std::size_t reserved_size_ = 0;
};

template<typename T>
auto sizeCounterGuardFactory(T &size_counter);

template<typename T>
class SizeCounterGuard
{
public:
  using size_counter_type  = T;

private:
  friend auto sizeCounterGuardFactory<T>(T &size_counter);

  T *size_counter_;

  SizeCounterGuard(T *size_counter) : size_counter_{size_counter}
  {
  }

  SizeCounterGuard(SizeCounterGuard const&) = delete;
  SizeCounterGuard &operator =(SizeCounterGuard const&) = delete;

public:
  SizeCounterGuard(SizeCounterGuard &&) = default;
  SizeCounterGuard &operator =(SizeCounterGuard &&) = delete;

  ~SizeCounterGuard()
  {
    if (size_counter_)
    {
      //* Resetting size counter to zero size by reconstructing it
      *size_counter_ = size_counter_type();
    }
  }

  bool is_unreserved() const
  {
    return size_counter_ && size_counter_->size() == 0;
  }
};

template<typename T>
auto sizeCounterGuardFactory(T &size_counter)
{
  return SizeCounterGuard<T>{ size_counter.size() == 0 ? &size_counter : nullptr };
}

}  // namespace serializers
}  // namespace fetch
