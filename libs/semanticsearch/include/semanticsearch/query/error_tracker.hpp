#pragma once

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

  operator bool()
  {
    return errors_.size() != 0;
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