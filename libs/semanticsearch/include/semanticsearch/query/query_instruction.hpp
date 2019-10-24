#pragma once

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"

#include <vector>

namespace fetch {
namespace semanticsearch {

struct QueryInstruction
{
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using ByteArray      = fetch::byte_array::ByteArray;
  using Token          = fetch::byte_array::Token;

  enum class Type
  {
    UNKNOWN = 0,
    SET_CONTEXT,
    PUSH_SCOPE,
    POP_SCOPE,

    FUNCTION = 50,
    EXECUTE_CALL,

    // Operators come first and are ordered according to precendence
    // That is: DO not change the order unless you intensionally
    // want to change the behaviour of the program.
    MULTIPLY = 100,
    ADD,
    SUB,

    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_THAN_EQUAL,
    MORE_THAN,
    MORE_THAN_EQUAL,

    SUBSCOPE,
    VAR_TYPE,
    ASSIGN,
    ATTRIBUTE,
    SEPARATOR,

    OBJECT_KEY,

    // Literals
    FLOAT,
    INTEGER,
    STRING,

    IDENTIFIER,

    // Only used for compilation
    INTERNAL_OPEN_GROUP,
    INTERNAL_CLOSE_GROUP
  };

  enum
  {
    PROP_NO_PROP = 0ull,
    PROP_CTX_MODEL,
    PROP_CTX_STORE,
    PROP_CTX_SET,
    PROP_CTX_FIND,

    PROP_IS_OPERATOR   = 1ul << 16,
    PROP_IS_GROUP      = 1ul << 17,
    PROP_IS_GROUP_OPEN = 1ul << 18,
    PROP_IS_CALL       = 1ul << 19
  };

  Type     type       = Type::UNKNOWN;
  uint64_t properties = PROP_NO_PROP;
  int      consumes   = 2;
  Token    token;
};

using CompiledStatement = std::vector<QueryInstruction>;

}  // namespace semanticsearch
}  // namespace fetch