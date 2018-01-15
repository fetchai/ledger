#ifndef BYTE_ARRAY_TOKENIZER_TOKEN_HPP
#define BYTE_ARRAY_TOKENIZER_TOKEN_HPP
#include <byte_array/referenced_byte_array.hpp>

namespace fetch {
namespace byte_array {

class Token : public ReferencedByteArray {
 public:
  Token() : ReferencedByteArray() {}

  Token(char const *str) : ReferencedByteArray(str) {}

  Token(std::string const &str) : ReferencedByteArray(str.c_str()) {}

  Token(ReferencedByteArray const &other) : ReferencedByteArray(other) {}

  Token(ReferencedByteArray const &other, std::size_t const &start,
        std::size_t const &length)
      : ReferencedByteArray(other, start, length) {}

  void SetType(std::size_t const &t) { type_ = t; }
  void SetLine(std::size_t const &l) { line_ = l; }
  void SetChar(std::size_t const &c) { char_ = c; }
  void SetFilename(ReferencedByteArray filename) { filename_ = filename; }

  std::size_t type() const { return type_; }
  std::size_t line() const { return line_; }
  std::size_t character() const { return char_; }
  ReferencedByteArray filename() const { return filename_; }

 private:
  std::size_t type_ = -1;
  std::size_t line_ = 0;
  std::size_t char_ = 0;
  ReferencedByteArray filename_;
};
};
};

#endif
