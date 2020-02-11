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

#include "semanticsearch/query/error_message.hpp"

#include <core/byte_array/byte_array.hpp>
#include <core/byte_array/const_byte_array.hpp>
#include <core/byte_array/consumers.hpp>
#include <core/byte_array/tokenizer/tokenizer.hpp>
#include <vector>

namespace fetch {
namespace semanticsearch {

class ErrorTracker
{
public:
  using ConstByteArray     = fetch::byte_array::ConstByteArray;
  using Token              = fetch::byte_array::Token;
  using SharedErrorTracker = std::shared_ptr<ErrorTracker>;

  ErrorTracker() = default;

  explicit operator bool()
  {
    return !errors_.empty();
  }

  void Print();
  void RaiseSyntaxError(ConstByteArray message, Token token);
  void RaiseRuntimeError(ConstByteArray message, Token token);
  void RaiseInternalError(ConstByteArray message, Token token);
  void SetSource(ConstByteArray source, ConstByteArray filename);
  void ClearErrors();
  bool HasErrors() const;

private:
  void PrintLine(int line, uint64_t character, uint64_t char_end = uint64_t(-1)) const;
  void PrintErrorMessage(ErrorMessage const &error);

  ConstByteArray            source_;
  ConstByteArray            filename_;
  std::vector<ErrorMessage> errors_;
};

}  // namespace semanticsearch
}  // namespace fetch
