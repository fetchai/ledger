#ifndef BYTE_ARRAY_TOKENIZER_TOKEN_HPP
#define BYTE_ARRAY_TOKENIZER_TOKEN_HPP
#include <byte_array/const_byte_array.hpp>

namespace fetch {
namespace byte_array {

class Token : public ConstByteArray {
 public:
  Token() : ConstByteArray() {}

  Token(char const *str) : ConstByteArray(str) {}

  Token(std::string const &str) : ConstByteArray(str.c_str()) {}

  Token(ConstByteArray const &other) : ConstByteArray(other) {}
  Token(ConstByteArray &&other) : ConstByteArray(other) {}  

  Token(ConstByteArray const &other, std::size_t const &start,
        std::size_t const &length)
      : ConstByteArray(other, start, length) {}

  void SetType(std::size_t const &t) { type_ = t; }
  void SetLine(std::size_t const &l) { line_ = l; }
  void SetChar(std::size_t const &c) { char_ = c; }

  std::size_t type() const { return type_; }
  std::size_t line() const { return line_; }
  std::size_t character() const { return char_; }


 private:
  int type_ = -1;
  std::size_t line_ = 0;
  std::size_t char_ = 0;
};
};
};

#endif
