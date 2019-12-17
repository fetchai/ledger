#pragma once

namespace fetch {
namespace semanticsearch {

struct Constants
{
  enum
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
    ASSIGN,
    VAR_TYPE,
    ATTRIBUTE,
    SEPARATOR,

    OBJECT_KEY,
    ADVERTISE_EXPIRY,

    // Literals
    LITERAL,
    IDENTIFIER,

    // Only used for compilation
    INTERNAL_OPEN_GROUP,
    INTERNAL_CLOSE_GROUP,

    STORE,
    STORE_POSITION,

    ADVERTISE,

    SEARCH_DIRECTION,
    SEARCH_GRANULARITY,
    SEARCH,

    USER_DEFINED_START = 300,

    //
    TYPE_NONE = 1ull << 21,
    TYPE_MODEL,
    TYPE_INSTANCE,
    TYPE_KEY,
    TYPE_FUNCTION_NAME,

  };
};

struct Properties
{
  enum
  {
    NONE = 0ull,
    PROP_CTX_MODEL,  // TODO: Rename
    PROP_CTX_ADVERTISE,
    PROP_CTX_SET,
    PROP_CTX_FIND,

    PROP_IS_OPERATOR   = 1ul << 16,
    PROP_IS_GROUP      = 1ul << 17,
    PROP_IS_GROUP_OPEN = 1ul << 18,
    PROP_IS_CALL       = 1ul << 19
  };
};

}  // namespace semanticsearch
}  // namespace fetch
