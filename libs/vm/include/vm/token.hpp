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

#include <cstdint>
#include <string>

namespace fetch {
namespace vm {

struct Token
{
  enum class Kind : uint16_t
  {
    EndOfInput = 0,
    Unknown,
    Integer8,
    UnsignedInteger8,
    Integer16,
    UnsignedInteger16,
    Integer32,
    UnsignedInteger32,
    Integer64,
    UnsignedInteger64,
    Float32,
    Float64,
    String,
    BadString,
    True,
    False,
    Null,
    Function,
    EndFunction,
    While,
    EndWhile,
    For,
    In,
    EndFor,
    If,
    ElseIf,
    Else,
    EndIf,
    Var,
    Return,
    Break,
    Continue,
    Identifier,
    AnnotationIdentifier,
    Comma,
    Dot,
    Colon,
    SemiColon,
    LeftParenthesis,
    RightParenthesis,
    LeftSquareBracket,
    RightSquareBracket,
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,
    Assign,
    InplaceAdd,
    InplaceSubtract,
    InplaceMultiply,
    InplaceDivide,
    InplaceModulo,
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    And,
    Or,
    Not,
    Inc,
    Dec
  };
  Kind        kind;
  uint32_t    offset;
  uint16_t    line;
  uint16_t    length;
  std::string text;
};

struct Location
{
  Location()
  {
    offset = 0;
    line   = 1;
  }
  uint32_t offset;
  uint16_t line;
};

}  // namespace vm
}  // namespace fetch
