#pragma once

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "core/commandline/vt100.hpp"

#include <vector>

namespace fetch {
namespace semanticsearch {

class ErrorMessage
{
public:
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using Token          = fetch::byte_array::Token;

  enum Type
  {
    WARNING,
    SYNTAX_ERROR,
    RUNTIME_ERROR,
    INTERNAL_ERROR,
    INFO,
    APPEND
  };

  ErrorMessage(ConstByteArray filename, ConstByteArray source, ConstByteArray message, Token token,
               Type type = SYNTAX_ERROR)
    : filename_(std::move(filename))
    , source_(std::move(source))
    , message_(std::move(message))
    , token_(std::move(token))
    , type_(std::move(type))
  {}

  ConstByteArray filename() const
  {
    return filename_;
  }
  ConstByteArray source() const
  {
    return source_;
  }
  ConstByteArray message() const
  {
    return message_;
  }
  Token token() const
  {
    return token_;
  }
  Type type() const
  {
    return type_;
  }
  int line() const
  {
    return token_.line();
  }
  uint64_t character() const
  {
    return token_.character();
  }

private:
  ConstByteArray filename_;
  ConstByteArray source_;
  ConstByteArray message_;
  Token          token_;
  Type           type_;
};

}  // namespace semanticsearch
}  // namespace fetch