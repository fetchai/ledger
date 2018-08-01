#pragma once
#include "core/logger.hpp"
#include "vectorise/memory/shared_array.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <type_traits>
namespace fetch {
namespace byte_array {
class ConstByteArray;
}

namespace serializers {  // TODO: refactor
template <typename T>
inline void Deserialize(T &, byte_array::ConstByteArray &);
}

namespace byte_array {

class ConstByteArray
{
public:
  typedef uint8_t                             container_type;
  typedef ConstByteArray                      self_type;
  typedef memory::SharedArray<container_type> shared_array_type;

  enum
  {
    NPOS = uint64_t(-1)
  };

  ConstByteArray() {}
  explicit ConstByteArray(std::size_t const &n) { Resize(n); }

  ConstByteArray(char const *str)
  {
    std::size_t n = 0;
    while (str[n] != '\0') ++n;
    Reserve(n);
    Resize(n);
    uint8_t const *up = reinterpret_cast<uint8_t const *>(str);
    for (std::size_t i = 0; i < n; ++i) data_[i] = up[i];
    //    data_[n] = '\0';
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

  // TODO: make explicit
  ConstByteArray(std::string const &s) : ConstByteArray(s.c_str()) {}

  ConstByteArray(self_type const &other)
      : data_(other.data_)
      , arr_pointer_(other.arr_pointer_)
      , start_(other.start_)
      , length_(other.length_)
  {}

  ConstByteArray(self_type const &other, std::size_t const &start, std::size_t const &length)
      : data_(other.data_), start_(start), length_(length)
  {
    assert(start_ + length_ <= data_.size());
    arr_pointer_ = data_.pointer() + start_;
  }

  ConstByteArray Copy() const
  {
    ConstByteArray ret;
    ret.Resize(size());
    for (std::size_t i = 0; i < size(); ++i) ret[i] = this->operator[](i);
    return ret;
  }

  ~ConstByteArray() = default;

  explicit operator std::string() const { return {char_pointer(), length_}; }

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
      if (arr_pointer_[i] != other.arr_pointer_[i]) break;
    if (i < n) return arr_pointer_[i] < other.arr_pointer_[i];
    return length_ < other.length_;
  }

  bool operator>(self_type const &other) const { return other < (*this); }

  bool operator==(self_type const &other) const
  {
    if (other.size() != size()) return false;
    bool ret = true;
    for (std::size_t i = 0; i < length_; ++i) ret &= (arr_pointer_[i] == other.arr_pointer_[i]);
    return ret;
  }

  bool operator!=(self_type const &other) const { return !(*this == other); }

  std::size_t capacity() const { return data_.size() == 0 ? 0 : data_.size() - 1; }

  bool operator==(char const *str) const
  {
    std::size_t i = 0;
    while ((str[i] != '\0') && (i < length_) && (str[i] == arr_pointer_[i])) ++i;
    return (str[i] == '\0') && (i == length_);
  }

  bool operator==(std::string const &s) const { return (*this) == s.c_str(); }

  bool operator!=(char const *str) const { return !(*this == str); }

  self_type SubArray(std::size_t const &start, std::size_t length = std::size_t(-1)) const
  {
    length = std::min(length, length_ - start);
    assert(start + length <= start_ + length_);
    return self_type(*this, start + start_, length);
  }

  bool Match(self_type const &str, std::size_t pos = 0) const
  {
    std::size_t p = 0;
    while ((pos < length_) && (p < str.size()) && (str[p] == arr_pointer_[pos])) ++pos, ++p;
    return (p == str.size());
  }

  bool Match(container_type const *str, std::size_t pos = 0) const
  {
    std::size_t p = 0;
    while ((pos < length_) && (str[p] != '\0') && (str[p] == arr_pointer_[pos])) ++pos, ++p;
    return (str[p] == '\0');
  }

  std::size_t Find(char const &c, std::size_t pos) const
  {
    while ((pos < length_) && (c != arr_pointer_[pos])) ++pos;
    if (pos >= length_) return NPOS;
    return pos;
  }

  std::size_t const &   size() const { return length_; }
  container_type const *pointer() const { return arr_pointer_; }

  char const *char_pointer() const { return reinterpret_cast<char const *>(arr_pointer_); }

  self_type operator+(self_type const &other) const
  {
    self_type ret;

    std::size_t n = 0, i = 0;
    ret.Resize(size() + other.size());
    for (; n < size(); ++n) ret[n] = this->operator[](n);
    for (; n < ret.size(); ++n, ++i) ret[n] = other[i];

    return ret;
  }

  int AsInt() const { return atoi(reinterpret_cast<char const *>(arr_pointer_)); }

  double AsFloat() const { return atof(reinterpret_cast<char const *>(arr_pointer_)); }

  // Non-const functions go here
  void FromByteArray(self_type const &other, std::size_t const &start, std::size_t length)
  {
    data_        = other.data_;
    start_       = other.start_ + start;
    length_      = length;
    arr_pointer_ = data_.pointer() + start_;
  }  // TODO: Move to protected

  container_type &operator[](std::size_t const &n)
  {
    assert(n < length_);
    return arr_pointer_[n];
  }


protected:
  void Resize(std::size_t const &n)
  {
    if (data_.size() < n) Reserve(n);
    length_ = n;
  }

  void Reserve(std::size_t const &n)
  {
    if (n <= data_.size())
    {
      return;
    }

    assert(n != 0);

    shared_array_type newdata(n);
    newdata.SetAllZero();

    std::size_t M = std::min(n, data_.size());
    std::memcpy(newdata.pointer(), data_.pointer(), M);

    data_        = newdata;
    arr_pointer_ = data_.pointer() + start_;
  }

  container_type *pointer() { return arr_pointer_; }

  char *char_pointer() { return reinterpret_cast<char *>(data_.pointer()); }

  template <typename T>
  friend void fetch::serializers::Deserialize(T &serializer, ConstByteArray &s);

private:
  shared_array_type data_;
  container_type *  arr_pointer_ = nullptr;
  std::size_t       start_ = 0, length_ = 0;
};

// typedef ConstByteArray BasicByteArray;

inline std::ostream &operator<<(std::ostream &os, ConstByteArray const &str)
{
  char const *arr = reinterpret_cast<char const *>(str.pointer());
  for (std::size_t i = 0; i < str.size(); ++i) os << arr[i];
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
