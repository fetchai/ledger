#pragma once
#include "core/byte_array/const_byte_array.hpp"

namespace fetch {
namespace byte_array {

class Token : public ConstByteArray
{
public:
  Token() : ConstByteArray() {}

  Token(char const *str) : ConstByteArray(str) {}

  Token(std::string const &str) : ConstByteArray(str.c_str()) {}

  Token(ConstByteArray const &other) : ConstByteArray(other) {}
  Token(ConstByteArray &&other) : ConstByteArray(other) {}

  Token(ConstByteArray const &other, std::size_t const &start, std::size_t const &length)
      : ConstByteArray(other, start, length)
  {}

  bool operator==(ConstByteArray const &other) const { return ConstByteArray::operator==(other); }

  bool operator!=(ConstByteArray const &other) const { return !(*this == other); }

  void SetType(int const &t) { type_ = t; }
  void SetLine(int const &l) { line_ = l; }
  void SetChar(std::size_t const &c) { char_ = c; }

  int         type() const { return type_; }
  int         line() const { return line_; }
  std::size_t character() const { return char_; }

private:
  int         type_ = -1;
  int         line_ = 0;
  std::size_t char_ = 0;
};
}  // namespace byte_array
}  // namespace fetch
