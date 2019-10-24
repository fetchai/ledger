#pragma once

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"

#include "semanticsearch/query/error_tracker.hpp"
#include "semanticsearch/query/query.hpp"

#include <vector>

namespace fetch {
namespace semanticsearch {

class QueryCompiler
{
public:
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using ByteArray      = fetch::byte_array::ByteArray;
  using Token          = fetch::byte_array::Token;

  QueryCompiler(ErrorTracker &error_tracker);
  Query operator()(ByteArray doc, ConstByteArray filename = "(internal)");

private:
  struct Statement
  {
    std::vector<Token> tokens;
    int                type;
  };

  std::vector<QueryInstruction> AssembleStatement(Statement const &stmt);
  void                          Tokenise();

  bool Match(ConstByteArray token);
  void SkipUntilEOL();
  void SkipWhitespaces();
  void SkipChar();
  void SkipChars(uint64_t const &length);

  void SkipUntil(uint8_t byte);

  ErrorTracker &error_tracker_;
  ByteArray     document_;
  uint64_t      position_{0};
  uint64_t      char_index_{0};
  int           line_{0};

  std::vector<Statement> statements_;

  std::vector<ConstByteArray> keywords_ = {"model", "store", "find", "var", "subspace", "schema"};
};

}  // namespace semanticsearch
}  // namespace fetch