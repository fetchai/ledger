#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace fetch {
namespace vm {

struct Token
{
  enum class Kind : uint16_t
  {
    EndOfInput = 0,
    Integer32,
    UnsignedInteger32,
    Integer64,
    UnsignedInteger64,
    SinglePrecisionNumber,
    DoublePrecisionNumber,
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
    Comma,
    Dot,
    Colon,
    SemiColon,
    LeftRoundBracket,
    RightRoundBracket,
    LeftSquareBracket,
    RightSquareBracket,
    Plus,
    Minus,
    Multiply,
    Divide,
    AddAssign,
    SubtractAssign,
    MultiplyAssign,
    DivideAssign,
    Assign,
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
    Dec,
    Unknown
  };
  class Hasher
  {
  public:
    size_t operator()(const Kind &key) const { return std::hash<uint16_t>{}((uint16_t)key); }
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
