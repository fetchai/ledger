#ifndef BYTE_ARRAY_REFERENCED_BYTE_ARRAY_HPP
#define BYTE_ARRAY_REFERENCED_BYTE_ARRAY_HPP

#include "byte_array/basic_byte_array.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <type_traits>
namespace fetch {
namespace byte_array {

class ByteArray : public BasicByteArray {
 public:
  typedef BasicByteArray super_type;

  ByteArray() {}
  ByteArray(char const *str) : super_type(str) {}
  ByteArray(std::string const &s) : super_type(s) {}
  ByteArray(ByteArray const &other) : super_type(other) {}
  ByteArray(std::initializer_list<container_type> l) : super_type(l) {}

  ByteArray(ByteArray const &other, std::size_t const &start,
            std::size_t const &length)
      : super_type(other, start, length) {}

  ByteArray(super_type const &other) : super_type(other) {}
  ByteArray(super_type const &other, std::size_t const &start,
            std::size_t const &length)
      : super_type(other, start, length) {}

  container_type &operator[](std::size_t const &n) {
    return super_type::operator[](n);
  }
  container_type const &operator[](std::size_t const &n) const {
    return super_type::operator[](n);
  }

  /*
   *
   * TODO: This breaks referencing - work out if this is the desired behaviour.
   */
  void Resize(std::size_t const &n) { return super_type::Resize(n); }

  /*
   *
   * TODO: This breaks referencing - work out if this is the desired behaviour.
   */
  void Reserve(std::size_t const &n) { return super_type::Reserve(n); }

  ByteArray operator+(ByteArray const &other) const {
    return super_type::operator+(other);
  }

  container_type const *pointer() const { return super_type::pointer(); }
  char const *char_pointer() const { return super_type::char_pointer(); }
  container_type *pointer() { return super_type::pointer(); }
  char *char_pointer() { return super_type::char_pointer(); }
};

inline std::ostream &operator<<(std::ostream &os, ByteArray const &str) {
  char const *arr = reinterpret_cast<char const *>(str.pointer());
  for (std::size_t i = 0; i < str.size(); ++i) os << arr[i];
  return os;
}

inline ByteArray operator+(char const *a, ByteArray const &b) {
  ByteArray s(a);
  s = s + b;
  return s;
}
}
}
#endif
