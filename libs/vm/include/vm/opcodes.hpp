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
#include <functional>

namespace fetch {
namespace vm {

using Opcode = uint16_t;

namespace Opcodes {
static Opcode const Unknown                      = 0;
static Opcode const VarDeclare                   = 1;
static Opcode const VarDeclareAssign             = 2;
static Opcode const PushConstant                 = 3;
static Opcode const PushString                   = 4;
static Opcode const PushNull                     = 5;
static Opcode const PushVariable                 = 6;
static Opcode const PushElement                  = 7;
static Opcode const PopToVariable                = 8;
static Opcode const PopToElement                 = 9;
static Opcode const Discard                      = 10;
static Opcode const Destruct                     = 11;
static Opcode const Break                        = 12;
static Opcode const Continue                     = 13;
static Opcode const Jump                         = 14;
static Opcode const JumpIfFalse                  = 15;
static Opcode const JumpIfTrue                   = 16;
static Opcode const Return                       = 17;
static Opcode const ReturnValue                  = 18;
static Opcode const ToInt8                       = 19;
static Opcode const ToByte                       = 20;
static Opcode const ToInt16                      = 21;
static Opcode const ToUInt16                     = 22;
static Opcode const ToInt32                      = 23;
static Opcode const ToUInt32                     = 24;
static Opcode const ToInt64                      = 25;
static Opcode const ToUInt64                     = 26;
static Opcode const ToFloat32                    = 27;
static Opcode const ToFloat64                    = 28;
static Opcode const ForRangeInit                 = 29;
static Opcode const ForRangeIterate              = 30;
static Opcode const ForRangeTerminate            = 31;
static Opcode const InvokeUserFunction           = 32;
static Opcode const Equal                        = 33;
static Opcode const ObjectEqual                  = 34;
static Opcode const NotEqual                     = 35;
static Opcode const ObjectNotEqual               = 36;
static Opcode const LessThan                     = 37;
static Opcode const ObjectLessThan               = 38;
static Opcode const LessThanOrEqual              = 39;
static Opcode const ObjectLessThanOrEqual        = 40;
static Opcode const GreaterThan                  = 41;
static Opcode const ObjectGreaterThan            = 42;
static Opcode const GreaterThanOrEqual           = 43;
static Opcode const ObjectGreaterThanOrEqual     = 44;
static Opcode const And                          = 45;
static Opcode const Or                           = 46;
static Opcode const Not                          = 47;
static Opcode const VariablePrefixInc            = 48;
static Opcode const VariablePrefixDec            = 49;
static Opcode const VariablePostfixInc           = 50;
static Opcode const VariablePostfixDec           = 51;
static Opcode const ElementPrefixInc             = 52;
static Opcode const ElementPrefixDec             = 53;
static Opcode const ElementPostfixInc            = 54;
static Opcode const ElementPostfixDec            = 55;
static Opcode const Modulo                       = 56;
static Opcode const VariableModuloAssign         = 57;
static Opcode const ElementModuloAssign          = 58;
static Opcode const UnaryMinus                   = 59;
static Opcode const ObjectUnaryMinus             = 60;
static Opcode const Add                          = 70;
static Opcode const LeftAdd                      = 71;
static Opcode const RightAdd                     = 72;
static Opcode const ObjectAdd                    = 73;
static Opcode const VariableAddAssign            = 74;
static Opcode const VariableRightAddAssign       = 75;
static Opcode const VariableObjectAddAssign      = 76;
static Opcode const ElementAddAssign             = 77;
static Opcode const ElementRightAddAssign        = 78;
static Opcode const ElementObjectAddAssign       = 79;
static Opcode const Subtract                     = 80;
static Opcode const LeftSubtract                 = 81;
static Opcode const RightSubtract                = 82;
static Opcode const ObjectSubtract               = 83;
static Opcode const VariableSubtractAssign       = 84;
static Opcode const VariableRightSubtractAssign  = 85;
static Opcode const VariableObjectSubtractAssign = 86;
static Opcode const ElementSubtractAssign        = 87;
static Opcode const ElementRightSubtractAssign   = 88;
static Opcode const ElementObjectSubtractAssign  = 89;
static Opcode const Multiply                     = 90;
static Opcode const LeftMultiply                 = 91;
static Opcode const RightMultiply                = 92;
static Opcode const ObjectMultiply               = 93;
static Opcode const VariableMultiplyAssign       = 94;
static Opcode const VariableRightMultiplyAssign  = 95;
static Opcode const VariableObjectMultiplyAssign = 96;
static Opcode const ElementMultiplyAssign        = 97;
static Opcode const ElementRightMultiplyAssign   = 98;
static Opcode const ElementObjectMultiplyAssign  = 99;
static Opcode const Divide                       = 100;
static Opcode const LeftDivide                   = 101;
static Opcode const RightDivide                  = 102;
static Opcode const ObjectDivide                 = 103;
static Opcode const VariableDivideAssign         = 104;
static Opcode const VariableRightDivideAssign    = 105;
static Opcode const VariableObjectDivideAssign   = 106;
static Opcode const ElementDivideAssign          = 107;
static Opcode const ElementRightDivideAssign     = 108;
static Opcode const ElementObjectDivideAssign    = 109;
static Opcode const NumReserved                  = 500;
}  // namespace Opcodes

class VM;
using OpcodeHandler = std::function<void(VM *)>;
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

}  // namespace vm
}  // namespace fetch
