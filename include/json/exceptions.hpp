#ifndef BYTE_ARRAY_JSON_EXCEPTIONS
#define BYTE_ARRAY_JSON_EXCEPTIONS

#include "byte_array/referenced_byte_array.hpp"
#include "byte_array/tokenizer/token.hpp"

#include <exception>
#include <sstream>
namespace fetch {
namespace json {

class UnrecognisedJSONSymbolException : public std::exception {
  std::string str_;

 public:
  UnrecognisedJSONSymbolException(byte_array::Token const& token) {
    std::stringstream msg;
    msg << "Unrecognised symbol '";
    msg << token << '\'' << " at line " << token.line() << ", character "
        << token.character() << "\n";
    str_ = msg.str();
  }

  virtual const char* what() const throw() { return str_.c_str(); }
};
};
};
#endif
