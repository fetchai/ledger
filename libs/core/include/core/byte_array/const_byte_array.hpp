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
#include "core/logger.hpp"
#include "vectorise/memory/shared_array.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <string.h>
#include <type_traits>

namespace fetch {
namespace byte_array {
class ConstByteArray;
}  // namespace byte_array

namespace serializers {
template <typename T>
inline void Deserialize(T &, byte_array::ConstByteArray &);
}  // namespace serializers

namespace byte_array {

class ConstByteArray
{
public:
  using container_type    = uint8_t;
  using self_type         = ConstByteArray;
  using shared_array_type = memory::SharedArray<container_type>;

  enum
  {
    NPOS = uint64_t(-1)
  };

  ConstByteArray() = default;

  explicit ConstByteArray(std::size_t const &n)
  {
    Resize(n);
  }

  ConstByteArray(char const *str)
    : ConstByteArray{reinterpret_cast<uint8_t const *>(str), str ? std::strlen(str) : 0}
  {
  }

  ConstByteArray(container_type const *const data, std::size_t const &size)
  {
    if(size > 0)
    {
      assert(data != nullptr);
      Reserve(size);
      Resize(size);
      WriteBytes(data, size);
    }
  }

  ConstByteArray(std::initializer_list<container_type> l)
  {
    Resize(l.size());
    std::size_t i = 0;
    for (auto &a : l)
    {
      data_[i++] = a;
    }
  }

  ConstByteArray(std::string const &s)
    : ConstByteArray(s.c_str())
  {}
  ConstByteArray(self_type const &other) = default;
  ConstByteArray(self_type &&other)      = default;
  // TODO(pbukva): (private issue #229: confusion what method does without analysing implementation
  // details - absolute vs relative[against `other.start_`] size)
  ConstByteArray(self_type const &other, std::size_t const &start, std::size_t const &length)
    : data_(other.data_)
    , start_(start)
    , length_(length)
    , arr_pointer_(data_.pointer() + start_)
  {
    assert(start_ + length_ <= data_.size());
  }

  ConstByteArray &operator=(ConstByteArray const &) = default;
  ConstByteArray &operator=(ConstByteArray &&other) = default;

  ConstByteArray Copy() const
  {
    return ConstByteArray{pointer(), size()};
  }

  void WriteBytes(container_type const *const src, std::size_t const &src_size, std::size_t const &dest_offset=0)
  {
    assert(dest_offset + src_size <= size());
    std::memcpy(pointer() + dest_offset, src, src_size);
  }

  void ReadBytes(container_type *const dest, std::size_t const &dest_size, std::size_t const &src_offset=0) const
  {
    assert(src_offset + dest_size <= size());
    std::memcpy(dest, pointer() + src_offset, dest_size);
  }

  ~ConstByteArray() = default;

  explicit operator std::string() const
  {
    return {char_pointer(), length_};
  }

  container_type const &operator[](std::size_t const &n) const
  {
    assert(n < length_);
    return arr_pointer_[n];
  }

  bool operator<(self_type const &other) const
  {
    std::size_t n = std::min(length_, other.length_);
    std::size_t i = 0;
    for (; i < n; ++i)
    {
      if (arr_pointer_[i] != other.arr_pointer_[i])
      {
        break;
      }
    }
    if (i < n)
    {
      return arr_pointer_[i] < other.arr_pointer_[i];
    }
    return length_ < other.length_;
  }

  bool operator>(self_type const &other) const
  {
    return other < (*this);
  }

  bool operator==(self_type const &other) const
  {
    if (other.size() != size())
    {
      return false;
    }
    bool ret = true;
    for (std::size_t i = 0; i < length_; ++i)
    {
      ret &= (arr_pointer_[i] == other.arr_pointer_[i]);
    }
    return ret;
  }

  bool operator!=(self_type const &other) const
  {
    return !(*this == other);
  }

  std::size_t capacity() const
  {
    // TODO(private issue #228: why `data_.size() - 1`?)
    // return data_.size() == 0 ? 0 : data_.size() - 1;
    return data_.size();
  }

  bool operator==(char const *str) const
  {
    std::size_t i = 0;
    while ((str[i] != '\0') && (i < length_) && (str[i] == arr_pointer_[i]))
    {
      ++i;
    }
    return (str[i] == '\0') && (i == length_);
  }

  bool operator==(std::string const &s) const
  {
    return (*this) == s.c_str();
  }

  bool operator!=(char const *str) const
  {
    return !(*this == str);
  }

public:
  self_type SubArray(std::size_t const &start, std::size_t length = std::size_t(-1)) const
  {
    return SubArray<self_type>(start, length);
  }

  bool Match(self_type const &str, std::size_t pos = 0) const
  {
    std::size_t p = 0;
    while ((pos < length_) && (p < str.size()) && (str[p] == arr_pointer_[pos]))
    {
      ++pos, ++p;
    }
    return (p == str.size());
  }

  bool Match(container_type const *str, std::size_t pos = 0) const
  {
    std::size_t p = 0;
    while ((pos < length_) && (str[p] != '\0') && (str[p] == arr_pointer_[pos]))
    {
      ++pos, ++p;
    }
    return (str[p] == '\0');
  }

  std::size_t Find(char const &c, std::size_t pos) const
  {
    while ((pos < length_) && (c != arr_pointer_[pos]))
    {
      ++pos;
    }
    if (pos >= length_)
    {
      return NPOS;
    }
    return pos;
  }

  std::size_t const &size() const
  {
    return length_;
  }
  container_type const *pointer() const
  {
    return arr_pointer_;
  }

  char const *char_pointer() const
  {
    return reinterpret_cast<char const *>(arr_pointer_);
  }

  self_type operator+(self_type const &other) const
  {
    self_type ret;
    ret.Append(*this, other);
    return ret;
  }

  int AsInt() const
  {
    return atoi(reinterpret_cast<char const *>(arr_pointer_));
  }

  double AsFloat() const
  {
    return atof(reinterpret_cast<char const *>(arr_pointer_));
  }

  // Non-const functions go here
  void FromByteArray(self_type const &other, std::size_t const &start, std::size_t length)
  {
    data_        = other.data_;
    start_       = other.start_ + start;
    length_      = length;
    arr_pointer_ = data_.pointer() + start_;
  }

  bool IsUnique() const noexcept
  {
    return data_.IsUnique();
  }

  uint64_t UseCount() const noexcept
  {
    return data_.UseCount();
  }

protected:
  template <typename RETURN_TYPE = self_type>
  RETURN_TYPE SubArray(std::size_t const &start, std::size_t length = std::size_t(-1)) const
  {
    length = std::min(length, length_ - start);
    assert(start + length <= start_ + length_);
    return RETURN_TYPE(*this, start + start_, length);
  }

  container_type &operator[](std::size_t const &n)
  {
    assert(n < length_);
    return arr_pointer_[n];
  }

  //* CEREFUL: The `resize_paradigm` operates in *SIZE* space for this method, which is always *RELATIVE* against the `start_` offset (as contrary to CAPACITY space - see the `Reserve(...)` bellow)
  //*   Meaning that if pradigm value is set to:
  //*    * `absolute`: then the `n` value represents ABSOLUTE *size* going to be set (new_size = n), which is internally still relative to `start_` offset,
  //*    * `relative`: then the `n` value represents (positive) RELATIVE increment of the *size* (new_size = old_size + n),
  //*   , where `new_size` is internally still relative to `start_` offset in *both* cases above.
  void Resize(std::size_t const &n, eResizeParadigm const resize_paradigm = eResizeParadigm::absolute, bool const zero_reserved_space=true)
  {
    std::size_t new_length;

    switch(resize_paradigm)
    {
      case eResizeParadigm::relative:
        new_length  = length_ + n;
        break;

      case eResizeParadigm::absolute:
        new_length  = n;
        break;
    }

    auto const new_capacity_for_reserve = start_ + new_length;

    Reserve(new_capacity_for_reserve, eResizeParadigm::absolute, zero_reserved_space);
    length_ = new_length;
  }

  //* CEREFUL: The `resize_paradigm` operates is in *CAPACITY* space for this method, which is *WHOLE* allocated size of underlying data buffer.
  //*   Meaning that if pradigm value is set to:
  //*    * `absolute`: then the `n` value represents ABSOLUTE *capacity* going to be set (new_capacity = n)
  //*    * `relative`: then the `n` value represents (positive) RELATIVE increment of the *capacity* (new_capacity = old_capacity + n)
  void Reserve(std::size_t const &n, eResizeParadigm const resize_paradigm = eResizeParadigm::absolute, bool const zero_reserved_space=true)
  {
    std::size_t new_capacity_for_reserve;

    switch(resize_paradigm)
    {
      case eResizeParadigm::relative:
        new_capacity_for_reserve = data_.size() + n;
        break;

      case eResizeParadigm::absolute:
        new_capacity_for_reserve = n;
        break;
    }

    if (new_capacity_for_reserve <= data_.size())
    {
      return;
    }

    assert(new_capacity_for_reserve != 0);

    shared_array_type newdata(new_capacity_for_reserve);
    std::memcpy(newdata.pointer(), data_.pointer(), data_.size());
    if (zero_reserved_space)
    {
      newdata.SetZeroAfter(data_.size());
    }

    data_        = newdata;
    arr_pointer_ = data_.pointer() + start_;
  }

  container_type *pointer()
  {
    return arr_pointer_;
  }

  char *char_pointer()
  {
    return reinterpret_cast<char *>(data_.pointer());
  }

  template <typename... Arg>
  self_type &Append(Arg const &... others)
  {
    AppendInternal(size(), others...);
    return *this;
  }

  template <typename T>
  friend void fetch::serializers::Deserialize(T &serializer, ConstByteArray &s);

private:
  void AppendInternal(std::size_t const acc_size)
  {
    Resize(acc_size);
  }

  template <typename... Arg>
  void AppendInternal(std::size_t const acc_size, self_type const &other, Arg const &... others)
  {
    AppendInternal(acc_size + other.size(), others...);
    std::memcpy(pointer() + acc_size, other.pointer(), other.size());
  }

  shared_array_type data_;
  std::size_t       start_ = 0, length_ = 0;
  container_type *  arr_pointer_ = nullptr;
};

inline std::ostream &operator<<(std::ostream &os, ConstByteArray const &str)
{
  char const *arr = reinterpret_cast<char const *>(str.pointer());
  for (std::size_t i = 0; i < str.size(); ++i)
  {
    os << arr[i];
  }
  return os;
}

inline ConstByteArray operator+(char const *a, ConstByteArray const &b)
{
  ConstByteArray s(a);
  s = s + b;
  return s;
}

}  // namespace byte_array
}  // namespace fetch
