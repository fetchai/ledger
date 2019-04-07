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
#include "meta/type_util.hpp"
#include "vectorise/memory/shared_array.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <ostream>
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

  explicit ConstByteArray(std::size_t n)
  {
    Resize(n);
  }

  ConstByteArray(char const *str)
    : ConstByteArray{reinterpret_cast<uint8_t const *>(str), str ? std::strlen(str) : 0}
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
    for (auto a : l)
    {
      data_[i++] = a;
    }
  }

  template <std::size_t n>
  ConstByteArray(std::array<container_type, n> const &a)
    : ConstByteArray(a.data(), n)
  {}

  ConstByteArray(std::string const &s)
    : ConstByteArray(reinterpret_cast<uint8_t const *>(s.data()), s.size())
  {}

  ConstByteArray(self_type const &other) = default;
  ConstByteArray(self_type &&other)      = default;
  // TODO(pbukva): (private issue #229: confusion what method does without analysing implementation
  // details - absolute vs relative[against `other.start_`] size)
  ConstByteArray(self_type const &other, std::size_t start, std::size_t length)
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
    int         retVal{std::memcmp(arr_pointer_, other.arr_pointer_, n)};

    return retVal < 0 || (retVal == 0 && length_ < other.length_);
  }

  bool operator>(self_type const &other) const
  {
    return other < (*this);
  }

  bool operator==(self_type const &other) const
  {
    return length_ == other.length_ && std::memcmp(arr_pointer_, other.arr_pointer_, length_) == 0;
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
    while ((i < length_) && (str[i] != '\0') && (str[i] == arr_pointer_[i]))
    {
      ++i;
    }
    return (i == length_) && (str[i] == '\0');
  }

  bool operator==(std::string const &s) const
  {
    return (*this) == s.c_str();
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
  self_type SubArray(std::size_t start, std::size_t length = std::size_t(-1)) const
  {
    return SubArray<self_type>(start, length);
  }

  bool Match(self_type const &str, std::size_t pos = 0) const
  {
    return length_ > pos && length_ - pos >= str.length_ &&
           std::memcmp(arr_pointer_ + pos, str.arr_pointer_, str.length_) == 0;
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

  std::size_t Find(char c, std::size_t pos) const
  {
    if (pos < length_)
    {
      auto retVal{
          static_cast<container_type const *>(std::memchr(arr_pointer_ + pos, c, length_ - pos))};
      if (retVal)
      {
        return static_cast<std::size_t>(retVal - arr_pointer_);
      }
    }
    return NPOS;
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
    return self_type{}.Append(*this, other);
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
  RETURN_TYPE SubArray(std::size_t start, std::size_t length = std::size_t(-1)) const
  {
    if (start > length_)
    {
      start = length_;
    }
    if (length > length_ - start)
    {
      length = length_ - start;
    }
    assert(start + length <= start_ + length_);
    return RETURN_TYPE(*this, start + start_, length);
  }

  container_type &operator[](std::size_t n)
  {
    assert(n < length_);
    return arr_pointer_[n];
  }

  /**
   * Resizes the array and allocates ammount of memory necessary to contain the requested size.
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
   * @zero_reserved_space If true then the ammount of new memory reserved/allocated (if any) ABOVE
   * of already allocated will be zeroed byte by byte.
   */
  void Resize(std::size_t const n, ResizeParadigm const resize_paradigm = ResizeParadigm::ABSOLUTE,
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
   * Reserves (allocates) requested ammount of memory IF it is more than already allocated.
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
   * @zero_reserved_space If true then the ammount of new memory reserved/allocated (if any) ABOVE
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
    AppendInternal<type_util::Const<self_type const &>::template type<Arg const &>...>(others...);
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
  template<class... Others> void AppendInternal(Others &&...others)
  {
    const std::size_t sz{size()};

    Resize(fetch::type_util::FoldL(std::plus<void>{}, size() + start_, others.size()...));

    fetch::type_util::FoldL(
	    [this](std::size_t acc, auto &&other){
		    std::memcpy(pointer() + acc, other.pointer(), other.size());
		    return acc + other.size();
	    }, sz, std::forward<Others>(others)...);
  }

  template <typename T>
  friend void fetch::serializers::Deserialize(T &serializer, ConstByteArray &s);

  shared_array_type data_;
  std::size_t       start_ = 0, length_ = 0;
  container_type *  arr_pointer_ = nullptr;
};

inline std::ostream &operator<<(std::ostream &os, ConstByteArray const &str)
{
  char const *arr = reinterpret_cast<char const *>(str.pointer());
  return os.write(arr, static_cast<std::streamsize>(str.size()));
}

inline ConstByteArray operator+(char const *a, ConstByteArray const &b)
{
  return ConstByteArray(a) + b;
}

inline ConstByteArray ToConstByteArray(ConstByteArray t) noexcept
{
  return t;
}

template <typename T>
inline ConstByteArray ToConstByteArray(T const &t)
{
  static_assert(std::is_same<ConstByteArray::container_type, char>::value ||
                    std::is_same<ConstByteArray::container_type, unsigned char>::value,
                "This library requires ConstByteArray::container_type to be implemented as char or "
                "unsigned char.");
  return ConstByteArray(reinterpret_cast<ConstByteArray::container_type const *>(&t), sizeof(T));
}

template <typename T>
inline T FromConstByteArray(ConstByteArray const &str, std::size_t offset = 0)
{
  static_assert(std::is_same<ConstByteArray::container_type, char>::value ||
                    std::is_same<ConstByteArray::container_type, unsigned char>::value,
                "This library requires ConstByteArray::container_type to be implemented as char or "
                "unsigned char.");
  T retVal;
  str.ReadBytes(reinterpret_cast<ConstByteArray::container_type *>(&retVal), sizeof(T), offset);
  return retVal;
}

}  // namespace byte_array
}  // namespace fetch
