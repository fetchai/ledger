#ifndef BYTE_ARRAY_REFERENCED_BYTE_ARRAY_HPP
#define BYTE_ARRAY_REFERENCED_BYTE_ARRAY_HPP

#include "memory/shared_array.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <type_traits>
namespace fetch {
namespace byte_array {

class ReferencedByteArray {
 public:
  typedef uint8_t container_type;
  typedef memory::SharedArray<container_type> shared_array_type;

  enum { NPOS = uint64_t(-1) };

  ReferencedByteArray() {}

  ReferencedByteArray(char const *str) {
    std::size_t n = 0;
    while (str[n] != '\0') ++n;
    Reserve(n);
    Resize(n);
    for (std::size_t i = 0; i < n; ++i) data_[i] = str[i];
    //    data_[n] = '\0';
  }

  ReferencedByteArray(std::string const &s) : ReferencedByteArray(s.c_str()) {}

  ReferencedByteArray(ReferencedByteArray const &other)
      : data_(other.data_),
        arr_pointer_(other.arr_pointer_),
        start_(other.start_),
        length_(other.length_) {}

  ReferencedByteArray(ReferencedByteArray const &other,
                      std::size_t const &start, std::size_t const &length)
      : data_(other.data_), start_(start), length_(length) {
    assert(start_ + length_ <= other.size());
    arr_pointer_ = data_.pointer() + start_;
  }

  ReferencedByteArray Copy() const {
    ReferencedByteArray ret;
    ret.Resize(size());
    for (std::size_t i = 0; i < size(); ++i) ret[i] = this->operator[](i);
    return ret;
  }

  virtual ~ReferencedByteArray() {}

  operator std::string() const {
    std::string ret;
    ret.resize(length_);
    for (std::size_t i = 0; i < length_; ++i) ret[i] = arr_pointer_[i];
    return ret;
  }

  container_type &operator[](std::size_t const &n) {
    assert(n < length_);
    return arr_pointer_[n];
  }

  container_type const &operator[](std::size_t const &n) const {
    assert(n < length_);
    return arr_pointer_[n];
  }

  bool operator<(ReferencedByteArray const &other) const {
    std::size_t n = std::min(length_, other.length_);
    std::size_t i = 0;
    for (; i < n; ++i)
      if (arr_pointer_[i] != other.arr_pointer_[i]) break;
    if (i < n) return arr_pointer_[i] < other.arr_pointer_[i];
    return length_ < other.length_;
  }

  bool operator==(ReferencedByteArray const &other) const {
    if (other.size() != size()) return false;
    bool ret = true;
    for (std::size_t i = 0; i < length_; ++i)
      ret &= (arr_pointer_[i] == other.arr_pointer_[i]);
    return ret;
  }

  bool operator!=(ReferencedByteArray const &other) const {
    return !(*this == other);
  }

  void Resize(std::size_t const &n) {
    if (data_.size() < n) Reserve(n);
    length_ = n;
  }

  virtual void Reserve(std::size_t const &n) {
    shared_array_type newdata(n);

    std::size_t M = std::min(n, data_.size());
    std::size_t i = 0;
    for (; i < M; ++i) newdata[i] = data_[i];

    //    for (; i < n + 1; ++i) newdata[i] = '\0';
    for (; i < n; ++i) newdata[i] = '\0';

    data_ = newdata;
    arr_pointer_ = data_.pointer() + start_;
  }

  std::size_t capacity() const {
    return data_.size() == 0 ? 0 : data_.size() - 1;
  }

  bool operator==(char const *str) const {
    std::size_t i = 0;
    while ((str[i] != '\0') && (i < length_) && (str[i] == arr_pointer_[i]))
      ++i;
    return (str[i] == '\0') && (i == length_);
  }

  bool operator!=(char const *str) const { return !(*this == str); }

  ReferencedByteArray SubArray(std::size_t const &start,
                               std::size_t length = std::size_t(-1)) const {
    length = std::min(length, length_ - start);
    assert(start + length <= start_ + length_);
    return ReferencedByteArray(*this, start + start_, length);
  }

  bool Match(ReferencedByteArray const &str, std::size_t const &p = 0) const {
    return Match(str.pointer(), p);
  }

  bool Match(container_type const *str, std::size_t pos = 0) const {
    std::size_t p = 0;
    while ((pos < length_) && (str[p] != '\0') && (str[p] == arr_pointer_[pos]))
      ++pos, ++p;
    return (str[p] == '\0');
  }

  std::size_t Find(char const &c, std::size_t pos) const {
    while ((pos < length_) && (c != arr_pointer_[pos])) ++pos;
    if (pos >= length_) return NPOS;
    return pos;
  }

  ReferencedByteArray operator+(ReferencedByteArray const &other) {
    ReferencedByteArray ret;

    std::size_t n = 0, i = 0;
    ret.Resize(size() + other.size());
    for (; n < size(); ++n) ret[n] = this->operator[](n);
    for (; n < ret.size(); ++n, ++i) ret[n] = other[i];

    return ret;
  }

  int AsInt() const {
    int n = 0;
    for (std::size_t i = 0; i < length_; ++i) {
      n *= 10;
      int a = (arr_pointer_[i] - '0');
      if ((a < 0) || (a > 9)) {
        std::cerr << "TODO: throw error - NaN in referenced byte_array: " << a
                  << " " << arr_pointer_[i] << std::endl;
        exit(-1);
      }
      n |= a;
    }
    return n;
  }

  std::size_t const &size() const { return length_; }
  container_type const *pointer() const { return arr_pointer_; }
  container_type *pointer() { return arr_pointer_; }

  char *char_pointer() { return reinterpret_cast<char *>(data_.pointer()); }
  char const *char_pointer() const {
    return reinterpret_cast<char const *>(data_.pointer());
  }

 private:
  shared_array_type data_;
  container_type *arr_pointer_ = nullptr;
  std::size_t start_ = 0, length_ = 0;
};

std::ostream &operator<<(std::ostream &os, ReferencedByteArray const &str) {
  char const *arr = reinterpret_cast<char const *>(str.pointer());
  for (std::size_t i = 0; i < str.size(); ++i) os << arr[i];
  return os;
}

ReferencedByteArray operator+(char const *a, ReferencedByteArray const &b) {
  ReferencedByteArray s(a);
  s = s + b;
  return s;
}
};
};
#endif
