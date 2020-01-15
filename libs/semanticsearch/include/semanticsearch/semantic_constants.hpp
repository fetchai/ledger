#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

    ASSIGN,
    VAR_TYPE,
    ATTRIBUTE,
    SEPARATOR,

    OBJECT_KEY,

    // Literals
    LITERAL,
    IDENTIFIER,

    // Only used for compilation
    INTERNAL_OPEN_GROUP,
    INTERNAL_CLOSE_GROUP,

    // Header
    UNTIL,
    GRANULARITY,
    VERSION,
    MAX_DEPTH,
    LIMIT,

    USING,
    SPECIFICATION,

    STORE,
    STORE_POSITION,

    ADVERTISE,
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

constexpr uint64_t const MAXIMUM_DEPTH = 20;
}  // namespace semanticsearch
}  // namespace fetch
