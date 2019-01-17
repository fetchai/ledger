#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include <functional>

namespace fetch {
namespace vm {

using Opcode = uint16_t;

namespace Opcodes
{
  static Opcode const Unknown                          = 0;

  static Opcode const VarDeclare                       = 1;
  static Opcode const VarDeclareAssign                 = 2;
  static Opcode const PushConstant                     = 3;
  static Opcode const PushString                       = 4;
  static Opcode const PushNull                         = 5;
  static Opcode const PushVariable                     = 6;
  static Opcode const PushElement                      = 7;
  static Opcode const PopToVariable                    = 8;
  static Opcode const PopToElement                     = 9;
  static Opcode const Discard                          = 10;
  static Opcode const Destruct                         = 11;
  static Opcode const Break                            = 12;
  static Opcode const Continue                         = 13;
  static Opcode const Jump                             = 14;
  static Opcode const JumpIfFalse                      = 15;
  static Opcode const JumpIfTrue                       = 16;
  static Opcode const Return                           = 17;
  static Opcode const ReturnValue                      = 18;
  static Opcode const ToInt8                           = 19;
  static Opcode const ToByte                           = 20;
  static Opcode const ToInt16                          = 21;
  static Opcode const ToUInt16                         = 22;
  static Opcode const ToInt32                          = 23;
  static Opcode const ToUInt32                         = 24;
  static Opcode const ToInt64                          = 25;
  static Opcode const ToUInt64                         = 26;
  static Opcode const ToFloat32                        = 27;
  static Opcode const ToFloat64                        = 28;
  static Opcode const ForRangeInit                     = 29;
  static Opcode const ForRangeIterate                  = 30;
  static Opcode const ForRangeTerminate                = 31;
  static Opcode const InvokeUserFunction               = 32;
  static Opcode const PrimitiveEqual                   = 33;
  static Opcode const ObjectEqual                      = 34;
  static Opcode const PrimitiveNotEqual                = 35;
  static Opcode const ObjectNotEqual                   = 36;
  static Opcode const PrimitiveLessThan                = 37;
  static Opcode const PrimitiveLessThanOrEqual         = 38;
  static Opcode const PrimitiveGreaterThan             = 39;
  static Opcode const PrimitiveGreaterThanOrEqual      = 40;
  static Opcode const And                              = 41;
  static Opcode const Or                               = 42;
  static Opcode const Not                              = 43;
  static Opcode const VariablePrefixInc                = 44;
  static Opcode const VariablePrefixDec                = 45;
  static Opcode const VariablePostfixInc               = 46;
  static Opcode const VariablePostfixDec               = 47;
  static Opcode const ElementPrefixInc                 = 48;
  static Opcode const ElementPrefixDec                 = 49;
  static Opcode const ElementPostfixInc                = 50;
  static Opcode const ElementPostfixDec                = 51;
  static Opcode const PrimitiveUnaryMinus              = 52;
  static Opcode const ObjectUnaryMinus                 = 53;

  static Opcode const PrimitiveAdd                     = 60;
  static Opcode const LeftAdd                          = 61;
  static Opcode const RightAdd                         = 62;
  static Opcode const ObjectAdd                        = 63;
  static Opcode const VariablePrimitiveAddAssign       = 64;
  static Opcode const VariableRightAddAssign           = 65;
  static Opcode const VariableObjectAddAssign          = 66;
  static Opcode const ElementPrimitiveAddAssign        = 67;
  static Opcode const ElementRightAddAssign            = 68;
  static Opcode const ElementObjectAddAssign           = 69;

  static Opcode const PrimitiveSubtract                = 70;
  static Opcode const LeftSubtract                     = 71;
  static Opcode const RightSubtract                    = 72;
  static Opcode const ObjectSubtract                   = 73;
  static Opcode const VariablePrimitiveSubtractAssign  = 74;
  static Opcode const VariableRightSubtractAssign      = 75;
  static Opcode const VariableObjectSubtractAssign     = 76;
  static Opcode const ElementPrimitiveSubtractAssign   = 77;
  static Opcode const ElementRightSubtractAssign       = 78;
  static Opcode const ElementObjectSubtractAssign      = 79;

  static Opcode const PrimitiveMultiply                = 80;
  static Opcode const LeftMultiply                     = 81;
  static Opcode const RightMultiply                    = 82;
  static Opcode const ObjectMultiply                   = 83;
  static Opcode const VariablePrimitiveMultiplyAssign  = 84;
  static Opcode const VariableRightMultiplyAssign      = 85;
  static Opcode const VariableObjectMultiplyAssign     = 86;
  static Opcode const ElementPrimitiveMultiplyAssign   = 87;
  static Opcode const ElementRightMultiplyAssign       = 88;
  static Opcode const ElementObjectMultiplyAssign      = 89;

  static Opcode const PrimitiveDivide                  = 90;
  static Opcode const LeftDivide                       = 91;
  static Opcode const RightDivide                      = 92;
  static Opcode const ObjectDivide                     = 93;
  static Opcode const VariablePrimitiveDivideAssign    = 94;
  static Opcode const VariableRightDivideAssign        = 95;
  static Opcode const VariableObjectDivideAssign       = 96;
  static Opcode const ElementPrimitiveDivideAssign     = 97;
  static Opcode const ElementRightDivideAssign         = 98;
  static Opcode const ElementObjectDivideAssign        = 99;

  static Opcode const NumReserved                      = 500;
}

class VM;
using OpcodeHandler = std::function<void (VM *)>;
struct OpcodeHandlerInfo
{
  OpcodeHandlerInfo(Opcode opcode__, OpcodeHandler handler__)
  {
    opcode  = opcode__;
    handler = handler__;
  }
  Opcode        opcode;
  OpcodeHandler handler;
};

} // namespace vm
} // namespace fetch
