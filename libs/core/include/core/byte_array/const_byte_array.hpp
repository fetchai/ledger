#ifndef BYTE_ARRAY_CONST_BYTE_ARRAY_HPP
#define BYTE_ARRAY_CONST_BYTE_ARRAY_HPP

#include "core/byte_array/basic_byte_array.hpp"
#include "core/byte_array/referenced_byte_array.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <type_traits>
namespace fetch {
namespace byte_array {

class ConstByteArray : public BasicByteArray {
 public:
  typedef BasicByteArray super_type;

  ConstByteArray() {}
  ConstByteArray(char const *str) : super_type(str) {}
  ConstByteArray(std::string const &s) : super_type(s) {}
  ConstByteArray(ConstByteArray const &other) : super_type(other) {}
  ConstByteArray(std::initializer_list<container_type> l) : super_type(l) {}

  ConstByteArray(ConstByteArray const &other, std::size_t const &start,
                 std::size_t const &length)
      : super_type(other, start, length) {}

  ConstByteArray(ByteArray const &other) : super_type(other) {}

  ConstByteArray(ByteArray const &other, std::size_t const &start,
                 std::size_t const &length)
      : super_type(other, start, length) {}

  ConstByteArray(super_type const &other) : super_type(other) {}
  ConstByteArray(super_type const &other, std::size_t const &start,
                 std::size_t const &length)
      : super_type(other, start, length) {}
};
}
}
#endif
