#ifndef BYTE_ARRAY_CONSUMERS_HPP
#define BYTE_ARRAY_CONSUMERS_HPP
#include "byte_array/const_byte_array.hpp"
#include "byte_array/tokenizer/tokenizer.hpp"

namespace fetch {
namespace byte_array {
namespace consumers {
typedef typename byte_array::Tokenizer::consumer_function_type
    consumer_function_type;

bool Word(byte_array::ConstByteArray const &str, uint64_t &pos) {
  while ((('a' <= str[pos]) && (str[pos] <= 'z')) ||
         (('A' <= str[pos]) && (str[pos] <= 'Z')) || (str[pos] == '\''))
    ++pos;
  return true;
}

bool AlphaNumeric(byte_array::ConstByteArray const &str, uint64_t &pos) {
  while ((('a' <= str[pos]) && (str[pos] <= 'z')) ||
         (('A' <= str[pos]) && (str[pos] <= 'Z')) ||
         (('0' <= str[pos]) && (str[pos] <= '9')))
    ++pos;
  return true;
}

bool AlphaNumericLetterFirst(byte_array::ConstByteArray const &str,
                             uint64_t &pos) {
  if (!((('a' <= str[pos]) && (str[pos] <= 'z')) ||
        (('A' <= str[pos]) && (str[pos] <= 'Z'))))
    return false;

  while ((('a' <= str[pos]) && (str[pos] <= 'z')) ||
         (('A' <= str[pos]) && (str[pos] <= 'Z')) ||
         (('0' <= str[pos]) && (str[pos] <= '9')))
    ++pos;
  return true;
}

consumer_function_type StringEnclosedIn(char c) {
  return
      [c](byte_array::ConstByteArray const &str, uint64_t &pos) -> bool {
        if (str[pos] != c) return false;
        ++pos;
        while ((str[pos] != c) && (str[pos] != '\0'))
          pos += (str[pos] == '\\') + 1;

        if (str[pos] == c) {
          ++pos;
          return true;
        }
        return false;
      };
}

consumer_function_type SingleChar(char c) {
  return
    [c](byte_array::ConstByteArray const &str, uint64_t &pos) -> bool {
    if (str[pos] == c) {
      ++pos;
      return true;     
    }
    return false;    
  };
  
}

consumer_function_type TokenFromList(std::vector<std::string> list) {
  return [list](byte_array::ConstByteArray const &str,
                uint64_t &pos) -> bool {
    for (auto &op : list)
      if (str.Match(op.c_str(), pos)) {
        pos += op.size();
        return true;
      }
    return false;
  };
}

consumer_function_type Keyword(std::string keyword) {
  return [keyword](byte_array::ConstByteArray const &str,
                   uint64_t &pos) -> bool {
    if (str.Match(keyword.c_str(), pos)) {
      pos += keyword.size();
      return true;
    }
    return false;
  };
}

bool Integer(byte_array::ConstByteArray const &str, uint64_t &pos) {
  uint64_t oldpos = pos;
  uint64_t N = pos + 1;
  if ((N < str.size()) && (str[pos] == '-') && ('0' <= str[N]) &&
      (str[N] <= '9'))
    pos += 2;

  while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;
  return oldpos != pos;
}

bool Float(byte_array::ConstByteArray const &str, uint64_t &pos) {
  uint64_t oldpos = pos;
  uint64_t N = pos + 1;
  if ((N < str.size()) && (str[pos] == '-') && ('0' <= str[N]) &&
      (str[N] <= '9'))
    pos += 2;

  while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;

  if ((pos < str.size()) && ('.' == str[pos])) ++pos;

  while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;

  if ((pos < str.size()) && (str[pos] == 'e')) {
    N = pos + 1;
    if ((N < str.size()) && (str[pos] == '-') && ('0' <= str[N]) &&
        (str[N] <= '9'))
      pos += 2;

    while ((pos < str.size()) && ('0' <= str[pos]) && (str[pos] <= '9')) ++pos;
  }

  return oldpos != pos;
}

bool Whitespace(byte_array::ConstByteArray const &str, uint64_t &pos) {
  bool ret = false;
  while ((pos < str.size()) && ((str[pos] == ' ') || (str[pos] == '\n') ||
                                (str[pos] == '\r') || (str[pos] == '\t')))
    ret = true, ++pos;
  return ret;
}

bool AnyChar(byte_array::ConstByteArray const &str, uint64_t &pos) {
  ++pos;
  return true;
}
};
};
};
#endif
