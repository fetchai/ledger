#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

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
    , type_(type)
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
