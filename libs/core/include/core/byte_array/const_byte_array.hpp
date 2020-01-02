#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/common.hpp"
#include "logging/logging.hpp"
#include "meta/value_util.hpp"
#include "vectorise/memory/shared_array.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

namespace fetch {
namespace byte_array {

class ConstByteArray
{
public:
  using ValueType       = uint8_t;
  using SelfType        = ConstByteArray;
  using SharedArrayType = memory::SharedArray<ValueType>;

  enum
  {
    NPOS = uint64_t(-1)
  };

  constexpr ConstByteArray() = default;

  explicit ConstByteArray(std::size_t n)
  {
    Resize(n);
  }

  ConstByteArray(char const *str)  // NOLINT
    : ConstByteArray{reinterpret_cast<uint8_t const *>(str), str != nullptr ? std::strlen(str) : 0}
  {}

  ConstByteArray(ValueType const *const data, std::size_t size)
  {
    if (size > 0)
    {
      assert(data != nullptr);
      Reserve(size);
      Resize(size);
      WriteBytes(data, size);
    }
  }

  ConstByteArray(std::initializer_list<ValueType> l)
  {
    Resize(l.size());
    std::size_t i = 0;
    for (auto &a : l)
    {
      data_[i++] = a;
    }
  }

  ConstByteArray(std::string const &s)  // NOLINT
    : ConstByteArray(reinterpret_cast<uint8_t const *>(s.data()), s.size())
  {}

  ConstByteArray(ConstByteArray const &other) = default;
  ConstByteArray(ConstByteArray &&other)      = default;

  // TODO(pbukva): (private issue #229: confusion what method does without analysing implementation
  // details - absolute vs relative[against `other.start_`] size)
  ConstByteArray(ConstByteArray const &other, std::size_t start, std::size_t length) noexcept
    : data_(other.data_)
    , start_(start)
    , length_(length)
    , arr_pointer_(data_.pointer() + start_)
  {
    assert(start_ + length_ <= data_.size());
  }

  explicit ConstByteArray(std::istream &in)
  {
    detailed_assert(in.good());

    in.seekg(0, std::ios::end);
    auto const size_in_bytes = in.tellg();
    in.seekg(0, std::ios::beg);

    Resize(static_cast<std::size_t>(size_in_bytes), ResizeParadigm::ABSOLUTE);

    in.read(char_pointer(), size_in_bytes);
  }

  ConstByteArray &operator=(ConstByteArray const &) = default;
  ConstByteArray &operator=(ConstByteArray &&other) = default;

  ConstByteArray Copy() const
  {
    return ConstByteArray{pointer(), size()};
  }

  void WriteBytes(ValueType const *const src, std::size_t src_size, std::size_t dest_offset = 0)
  {
    assert(dest_offset + src_size <= size());
    std::memcpy(pointer() + dest_offset, src, src_size);
  }

  void ReadBytes(ValueType *const dest, std::size_t dest_size, std::size_t src_offset = 0) const
  {
    if (src_offset + dest_size > size())
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "ReadBytes target array is too big for us to fill. dest_size=", dest_size,
                     " src_offset=", src_offset, " size=", size());

      throw std::range_error("ReadBytes target array is too big");
    }
    std::memcpy(dest, pointer() + src_offset, dest_size);
  }

  ~ConstByteArray() = default;

  explicit operator std::string() const
  {
    // Avoiding undefined behaviour (C++ standard defines passing nullptr as
    // input to `std::basic_string(char const*, ...)` as undefined behaviour):
    if (pointer() != nullptr)
    {
      return {char_pointer(), length_};
    }

    return {};
  }

  constexpr ValueType const &operator[](std::size_t n) const noexcept
  {
    assert(n < length_);
    return arr_pointer_[n];
  }

  bool operator<=(SelfType const &other) const
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
      return arr_pointer_[i] <= other.arr_pointer_[i];
    }
    return length_ <= other.length_;
  }

  bool operator<(SelfType const &other) const
  {
    if (length_ == 0)
    {
      return other.length_ != 0;
    }
    if (other.length_ == 0)
    {
      return false;
    }
    if (length_ < other.length_)
    {
      return std::memcmp(arr_pointer_, other.arr_pointer_, length_) <= 0;
    }
    return std::memcmp(arr_pointer_, other.arr_pointer_, other.length_) < 0;
  }

  bool operator>(SelfType const &other) const
  {
    return other < (*this);
  }

  bool operator>=(SelfType const &other) const
  {
    return other <= (*this);
  }

  bool operator==(SelfType const &other) const
  {
    return length_ == other.length_ &&
           (length_ == 0 || std::memcmp(arr_pointer_, other.arr_pointer_, length_) == 0);
  }

  bool operator!=(SelfType const &other) const
  {
    return !(*this == other);
  }

  constexpr std::size_t capacity() const noexcept
  {
    return data_.size();
  }

  bool operator==(char const *str) const
  {
    std::size_t i = 0;
    while (i < length_ && str[i] != '\0' && str[i] == arr_pointer_[i])
    {
      ++i;
    }
    return (i == length_) && (str[i] == '\0');
  }

  bool operator==(std::string const &s) const
  {
    return length_ == s.size() && std::memcmp(arr_pointer_, s.c_str(), length_) == 0;
  }

  bool operator!=(char const *str) const
  {
    return !(*this == str);
  }

  bool operator!=(std::string const &s) const
  {
    return !(*this == s);
  }

public:
  SelfType SubArray(std::size_t start, std::size_t length = std::size_t(-1)) const noexcept
  {
    return SubArrayInternal<SelfType>(start, length);
  }

  constexpr bool Match(SelfType const &str, std::size_t pos = 0) const noexcept
  {
    return pos + str.length_ <= length_ &&
           std::memcmp(arr_pointer_ + pos, str.arr_pointer_, str.length_) == 0;
  }

  constexpr bool Match(ValueType const *str, std::size_t pos = 0) const noexcept
  {
    std::size_t p = 0;
    while ((pos < length_) && (str[p] != '\0') && (str[p] == arr_pointer_[pos]))
    {
      ++pos, ++p;
    }
    return (str[p] == '\0');
  }

  std::size_t Find(char c, std::size_t pos) const
  {
    auto position =
        static_cast<ValueType const *>(std::memchr(arr_pointer_ + pos, c, length_ - pos));
    return position != nullptr ? static_cast<std::size_t>(position - arr_pointer_) : NPOS;
  }

  constexpr std::size_t size() const noexcept
  {
    return length_;
  }

  constexpr bool empty() const noexcept
  {
    return 0 == length_;
  }

  constexpr ValueType const *pointer() const noexcept
  {
    return arr_pointer_;
  }

  char const *char_pointer() const noexcept
  {
    return reinterpret_cast<char const *>(arr_pointer_);
  }

  SelfType operator+(SelfType const &other) const
  {
    SelfType ret = this->Copy();
    ret.Resize(other.size() + size());
    std::memcpy(ret.pointer() + size(), other.pointer(), other.size());
    return ret;
  }

  int AsInt() const
  {
    std::string const value{*this};

    auto const ret = std::strtol(value.c_str(), nullptr, 10);
    if (errno == ERANGE)
    {
      errno = 0;
      FETCH_LOG_ERROR(LOGGING_NAME, "AsInt() failed to convert value=", value, " to integer");

      throw std::domain_error("AsInt() failed to convert value=" + value + " to integer");
    }

    return static_cast<int>(ret);
  }

  double AsFloat() const
  {
    std::string const value{*this};

    auto const ret = std::strtod(value.c_str(), nullptr);
    if (errno == ERANGE)
    {
      errno = 0;
      FETCH_LOG_ERROR(LOGGING_NAME, "AsFloat() failed to convert value=", value, " to double");

      throw std::domain_error("AsFloat() failed to convert value=" + value + " to double");
    }

    return ret;
  }

  ConstByteArray ToBase64() const;
  ConstByteArray ToHex() const;
  ConstByteArray FromBase64() const;
  ConstByteArray FromHex() const;

  // Non-const functions go here
  void FromByteArray(SelfType const &other, std::size_t start, std::size_t length) noexcept
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
  template <typename ReturnType = SelfType>
  ReturnType SubArrayInternal(std::size_t start, std::size_t length = std::size_t(-1)) const
      noexcept
  {
    length = std::min(length, length_ - start);
    assert(start + length <= start_ + length_);
    return ReturnType(*this, start + start_, length);
  }

  constexpr ValueType &operator[](std::size_t n) noexcept
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
    std::size_t const new_length = resize_paradigm == ResizeParadigm::ABSOLUTE ? n : length_ + n;

    auto const new_capacity_for_reserve = start_ + new_length;

    Reserve(new_capacity_for_reserve, ResizeParadigm::ABSOLUTE, zero_reserved_space);
    length_ = new_length;
  }

  /**
   * Reserves (allocates) requested amount of memory IF it is more than already allocated.
   *
   * Note that this method operates in CAPACITY space, which is defined by WHOLE
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
  void Reserve(std::size_t n, ResizeParadigm const resize_paradigm = ResizeParadigm::ABSOLUTE,
               bool const zero_reserved_space = true)
  {
    std::size_t const new_capacity_for_reserve =
        resize_paradigm == ResizeParadigm::ABSOLUTE ? n : data_.size() + n;

    if (new_capacity_for_reserve <= data_.size())
    {
      return;
    }

    assert(new_capacity_for_reserve != 0);

    SharedArrayType newdata(new_capacity_for_reserve);
    std::memcpy(newdata.pointer(), data_.pointer(), data_.size());
    if (zero_reserved_space)
    {
      newdata.SetZeroAfter(data_.size());
    }

    data_        = newdata;
    arr_pointer_ = data_.pointer() + start_;
  }

  constexpr ValueType *pointer() noexcept
  {
    return arr_pointer_;
  }

  char *char_pointer() noexcept
  {
    return reinterpret_cast<char *>(data_.pointer());
  }

  template <typename... Arg>
  SelfType &Append(Arg &&... others)
  {
    AppendInternal<AppendedType<Arg>...>(static_cast<AppendedType<Arg>>(others)...);
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

      (*this)[pos] = static_cast<ValueType>(with);
      ++num_of_replacements;
    }
    return num_of_replacements;
  }

private:
  constexpr static char const *LOGGING_NAME = "ConstByteArray";

  /**
   * AddSize is a binary callable object that, when called,
   * returns the sum of its first argument and the size of the second.
   * By default, `size' is whatever returned by arg.size().
   * As a special case, the second argument can be a char, an int8_t or uint8_t,
   * in which case its size is always 1.
   */
  struct AddSize
  {
  public:
    template <class Arg>
    constexpr std::size_t operator()(std::size_t counter, Arg &&arg) noexcept(
        noexcept(static_cast<std::size_t>(std::declval<Arg>().size())))
    {
      return counter + static_cast<std::size_t>(std::forward<Arg>(arg).size());
    }

    constexpr std::size_t operator()(std::size_t counter, uint8_t /*unused*/) noexcept
    {
      return counter + 1;
    }

    constexpr std::size_t operator()(std::size_t counter, int8_t /*unused*/) noexcept
    {
      return counter + 1;
    }

    constexpr std::size_t operator()(std::size_t counter, char /*unused*/) noexcept
    {
      return counter + 1;
    }
  };

  /**
   * AddBytes is a binary callable object that keeps a reference to this bytearray.
   * It accepts two arguments: the offset and a anoher bytearray,
   * and pastes its second argument's contents into this bytearray starting at offset.
   * As a special case, the second argument can be a char, an int8_t or uint8_t,
   * in which case it is simply put into array at offset.
   * Returns the sum of its first argument and the size of the second, i.e. next offset
   * past the last copied byte, to be used in left-folds.
   */
  class AddBytes
  {
    SelfType &self_;

  public:
    explicit AddBytes(SelfType &self)
      : self_(self)
    {}

    template <class Arg>
    constexpr std::size_t operator()(std::size_t counter, Arg &&arg) noexcept(
        noexcept(std::memcpy(self_.pointer() + counter, arg.pointer(), arg.size())) &&
        noexcept(std::forward<Arg>(arg).size()))
    {
      std::memcpy(self_.pointer() + counter, arg.pointer(), arg.size());
      return counter + std::forward<Arg>(arg).size();
    }

    constexpr std::size_t operator()(std::size_t counter, uint8_t arg) noexcept
    {
      self_.pointer()[counter] = arg;
      return counter + 1;
    }

    constexpr std::size_t operator()(std::size_t counter, int8_t arg) noexcept
    {
      self_.pointer()[counter] = static_cast<uint8_t>(arg);
      return counter + 1;
    }

    constexpr std::size_t operator()(std::size_t counter, char arg) noexcept
    {
      self_.pointer()[counter] = static_cast<uint8_t>(arg);
      return counter + 1;
    }
  };

  template <typename T>
  using AppendedType =
      std::conditional_t<type_util::IsAnyOfV<std::decay_t<T>, uint8_t, char, int8_t>,
                         std::decay_t<T>, SelfType const &>;

  /**
   * Appends args to this array in left-to-right order.
   * Note that Args, as invoked by Append() are either single-byte scalars,
   * or const references, so no ref-qualification in the prototype.
   */
  template <typename... Args>
  void AppendInternal(Args... args)
  {
    auto old_size{size()};
    // grow enough to contain all the arguments
    Resize(value_util::Accumulate(AddSize{}, old_size, args...));
    // write down arguments' contents
    value_util::Accumulate(AddBytes{*this}, old_size, args...);
  }

  SharedArrayType data_;
  std::size_t     start_{0}, length_{0};
  ValueType *     arr_pointer_{nullptr};
};

std::ostream & operator<<(std::ostream &os, ConstByteArray const &str);
ConstByteArray operator+(char const *a, ConstByteArray const &b);

}  // namespace byte_array
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::byte_array::ConstByteArray>
{
  std::size_t operator()(fetch::byte_array::ConstByteArray const &value) const noexcept
  {
    uint64_t h = 2166136261U;

    for (uint64_t i = 0; i < value.size(); ++i)
    {
      h = (h * 16777619) ^ value[i];
    }

    return h;
  }
};

template <>
struct hash<fetch::byte_array::ByteArray> : public hash<fetch::byte_array::ConstByteArray>
{
};

}  // namespace std
