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

  explicit QueryCompiler(ErrorTracker &error_tracker);
  Query operator()(ByteArray doc, ConstByteArray const &filename = "(internal)");

private:
  struct Statement
  {
    std::vector<Token> tokens;
    int                type;
  };

  std::vector<QueryInstruction> AssembleStatement(Statement const &stmt);
  void                          Tokenise();

  bool Match(ConstByteArray const &token) const;
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
