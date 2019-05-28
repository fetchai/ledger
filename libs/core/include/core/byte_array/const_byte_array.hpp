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

#include "core/common.hpp"
#include "core/logger.hpp"
#include "meta/value_util.hpp"
#include "vectorise/memory/shared_array.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <ostream>
#include <string.h>
#include <type_traits>

namespace fetch {
namespace byte_array {

class ConstByteArray
{
public:
  using container_type    = std::uint8_t;
  using self_type         = ConstByteArray;
  using shared_array_type = memory::SharedArray<container_type>;

  enum
  {
    NPOS = uint64_t(-1)
  };

  ConstByteArray() = default;

  explicit ConstByteArray(std::size_t n)
  {
    Resize(n);
  }

  ConstByteArray(char const *str)
    : ConstByteArray{reinterpret_cast<std::uint8_t const *>(str), str ? std::strlen(str) : 0}
  {}

  ConstByteArray(container_type const *const data, std::size_t size)
  {
    if (size > 0)
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
    : ConstByteArray(reinterpret_cast<std::uint8_t const *>(s.data()), s.size())
  {}

  ConstByteArray(ConstByteArray const &other) = default;
  ConstByteArray(ConstByteArray &&other)      = default;
  // TODO(pbukva): (private issue #229: confusion what method does without analysing implementation
  // details - absolute vs relative[against `other.start_`] size)
  ConstByteArray(ConstByteArray const &other, std::size_t start, std::size_t length)
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

  void WriteBytes(container_type const *const src, std::size_t src_size,
                  std::size_t dest_offset = 0)
  {
    assert(dest_offset + src_size <= size());
    std::memcpy(pointer() + dest_offset, src, src_size);
  }

  void ReadBytes(container_type *const dest, std::size_t dest_size,
                 std::size_t src_offset = 0) const
  {
    if (src_offset + dest_size > size())
    {
      FETCH_LOG_WARN("ConstByteArray",
                     "ReadBytes target array is too big for us to fill. dest_size=", dest_size,
                     " src_offset=", src_offset, " size=", size());
      throw std::range_error("ReadBytes target array is too big");
    }
    std::memcpy(dest, pointer() + src_offset, dest_size);
  }

  ~ConstByteArray() = default;

  explicit operator std::string() const
  {
    return {char_pointer(), length_};
  }

  container_type const &operator[](std::size_t n) const
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
  self_type SubArray(std::size_t start, std::size_t length = std::size_t(-1)) const
  {
    return SubArrayInternal<self_type>(start, length);
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

  std::size_t size() const
  {
    return length_;
  }

  bool empty() const
  {
    return 0 == length_;
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
    std::string const value = static_cast<std::string>(*this);
    return atoi(value.c_str());
  }

  double AsFloat() const
  {
    std::string const value = static_cast<std::string>(*this);
    return atof(value.c_str());
  }

  ConstByteArray ToBase64() const;
  ConstByteArray ToHex() const;

  // Non-const functions go here
  void FromByteArray(self_type const &other, std::size_t start, std::size_t length)
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
  RETURN_TYPE SubArrayInternal(std::size_t start, std::size_t length = std::size_t(-1)) const
  {
    length = std::min(length, length_ - start);
    assert(start + length <= start_ + length_);
    return RETURN_TYPE(*this, start + start_, length);
  }

  container_type &operator[](std::size_t n)
  {
    assert(n < length_);
    return arr_pointer_[n];
  }

  /**
   * Resizes the array and allocates amount of memory necessary to contain the requested size.
   * Memory allocation is handled by the @ref Reserve() method.
   *
   * Please be NOTE, that this method operates in SIZE space, which is always RELATIVE
   * against the @ref start_ offset (as contrary to CAPACITY space - see the @ref Reserve() method)
   * Also, this method can operate in two modes - absolute(default) and relative, please see
   * description for @ref resize_paradigm parameter for more details
   *
   * @param n Requested size, is relative or absolute depending on the @ref resize_paradigm
   * parameter
   *
   * @param resize_paradigm Defines mode of resize operation. When set to ABSOLUTE value, array size
   * is going to be se to @ref n value. When set to RELATIVE value, array SIZE is going to be se to
   * original_size + n. Where new resulting SIZE is internally still relative to the internal start_
   * offset in BOTH cases (relative and absolute).
   *
   * @zero_reserved_space If true then the amount of new memory reserved/allocated (if any) ABOVE
   * of already allocated will be zeroed byte by byte.
   */
  void Resize(std::size_t n, ResizeParadigm const resize_paradigm = ResizeParadigm::ABSOLUTE,
              bool const zero_reserved_space = true)
  {
    std::size_t new_length{0};

    switch (resize_paradigm)
    {
    case ResizeParadigm::RELATIVE:
      new_length = length_ + n;
      break;

    case ResizeParadigm::ABSOLUTE:
      new_length = n;
      break;
    }

    auto const new_capacity_for_reserve = start_ + new_length;

    Reserve(new_capacity_for_reserve, ResizeParadigm::ABSOLUTE, zero_reserved_space);
    length_ = new_length;
  }

  /**
   * Reserves (allocates) requested amount of memory IF it is more than already allocated.
   *
   * Please be NOTE, that this method operates in CAPACITY space, which is defined by WHOLE
   * allocated size of underlying data buffer.
   *
   * @param n Requested capacity, is relative or absolute depending on the @ref resize_paradigm
   * parameter
   *
   * @param resize_paradigm Defines mode of resize operation. When set to ABSOLUTE value, then
   * CAPACITY of WHOLE underlying array (allocated memory) is going to be set to @ref n value IF
   * requested @ref n value is bigger than current CAPACITY of the the array. When set to RELATIVE
   * value, then capacity of of WHOLE underlying array (allocated memory) is going to be set to
   * current_capacity + n, what ALWAYS resuts to re-allocation since the requested CAPACITY is
   * always bigger then the current one.
   *
   * @zero_reserved_space If true then the amount of new memory reserved/allocated (if any) ABOVE
   * of already allocated will be zeroed byte by byte.
   */
  void Reserve(std::size_t   n,
               ResizeParadigm const resize_paradigm     = ResizeParadigm::ABSOLUTE,
               bool const           zero_reserved_space = true)
  {
    std::size_t new_capacity_for_reserve{0};

    switch (resize_paradigm)
    {
    case ResizeParadigm::RELATIVE:
      new_capacity_for_reserve = data_.size() + n;
      break;

    case ResizeParadigm::ABSOLUTE:
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

  constexpr container_type *pointer() noexcept
  {
    return arr_pointer_;
  }

  constexpr char *char_pointer() noexcept
  {
    return reinterpret_cast<char *>(data_.pointer());
  }

  template <typename... Arg>
  self_type &Append(Arg &&...others)
  {
    AppendInternal<AppendedType<Arg>...>(AppendedType<Arg>(others)...);
    return *this;
  }

  std::size_t Replace(char const &what, char const &with)
  {
    std::size_t num_of_replacements = 0;
    std::size_t pos                 = 0;
    while (pos < length_)
    {
      pos = Find(what, pos);
      if (pos == NPOS)
      {
        break;
      }

      (*this)[pos] = static_cast<container_type>(with);
      ++num_of_replacements;
    }
    return num_of_replacements;
  }

private:
  // this struct accumulates the size of all appended arguments
  struct GetSize {
  public:
	  template<class Arg> constexpr std::size_t operator()(std::size_t counter, Arg &&arg)
		  noexcept(noexcept(static_cast<std::size_t>(std::declval<Arg>().size())))
	  {
		  return counter + static_cast<std::size_t>(std::forward<Arg>(arg).size());
	  }
	  constexpr std::size_t operator()(std::size_t counter, std::uint8_t) noexcept { return counter + 1; }
	  constexpr std::size_t operator()(std::size_t counter, std::int8_t) noexcept { return counter + 1; }
	  constexpr std::size_t operator()(std::size_t counter, char) noexcept { return counter + 1; }
  };

  // this struct appends an argument's content to this bytearray
  class AddBytes {
	  self_type &self_;
  public:
	  AddBytes(self_type &self): self_(self) {}

	  template<class Arg> constexpr std::size_t operator()(std::size_t counter, Arg &&arg)
		  noexcept(noexcept(std::size_t(std::declval<Arg>().size())))
	  {
		  std::memcpy(self_.pointer() + counter, arg.pointer(), arg.size());
		  return counter + std::forward<Arg>(arg).size();
	  }
	  constexpr std::size_t operator()(std::size_t counter, std::uint8_t arg) noexcept {
		  self_.pointer()[counter] = arg;
		  return counter + 1;
	  }
	  constexpr std::size_t operator()(std::size_t counter, std::int8_t arg) noexcept {
		  self_.pointer()[counter] = static_cast<std::uint8_t>(arg);
		  return counter + 1;
	  }
	  constexpr std::size_t operator()(std::size_t counter, char arg) noexcept {
		  self_.pointer()[counter] = static_cast<std::uint8_t>(arg);
		  return counter + 1;
	  }
  };

  template<typename T> using AppendedType = std::conditional_t<
	  type_util::IsAnyOfV<std::decay_t<T>, std::uint8_t, char, unsigned char, std::int8_t>
	  , std::decay_t<T>
	  , self_type>;

  template<typename... Args> void AppendInternal(Args const &...args) {
	  auto old_size{size()};
	  // grow enough to contain all the arguments
	  Resize(value_util::Accumulate(GetSize{}, old_size, args...));
	  // write down arguments' contents
	  value_util::Accumulate(AddBytes{*this}, old_size, args...);
  }

  /*
  void AppendInternal(std::size_t const acc_size)
  {
    Resize(acc_size);
  }

  // TODO(pbukva) (private issue #257)
  template <typename... Args>
  void AppendInternal(std::size_t const acc_size, self_type const &other, Args &&... args)
  {
    AppendInternal(acc_size + other.size(), std::forward<Args>(args)...);
    memcpy(pointer() + acc_size, other.pointer(), other.size());
           //static_cast<size_t>(other.size()) & 0x7FFFFFFFFFFFFFFFull);
  }

  template <typename... Args>
  void AppendInternal(std::size_t const acc_size, self_type &&other, Args &&... args)
  {
    AppendInternal(acc_size + other.size(), std::forward<Args>(args)...);
    memcpy(pointer() + acc_size, other.pointer(), other.size());
           //static_cast<size_t>(other.size()) & 0x7FFFFFFFFFFFFFFFull);
  }

  template <typename... Args>
  void AppendInternal(std::size_t const acc_size, std::uint8_t other, Args &&... args)
  {
    AppendInternal(acc_size + 1, std::forward<Args>(args)...);
    pointer()[acc_size] = other;
  }
  */

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
