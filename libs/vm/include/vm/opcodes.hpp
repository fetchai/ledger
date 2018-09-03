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

namespace fetch {
namespace vm {

enum class Opcode : uint16_t
{
  Unknown,
  Jump,
  JumpIfFalse,
  PushString,
  PushConstant,
  PushVariable,
  VarDeclare,
  VarDeclareAssign,
  Assign,
  Discard,
  Destruct,
  InvokeUserFunction,
  ForRangeInit,
  ForRangeIterate,
  ForRangeTerminate,
  Break,
  Continue,
  Return,
  ReturnValue,
  ToInt8,
  ToByte,
  ToInt16,
  ToUInt16,
  ToInt32,
  ToUInt32,
  ToInt64,
  ToUInt64,
  ToFloat32,
  ToFloat64,
  EqualOp,
  NotEqualOp,
  LessThanOp,
  LessThanOrEqualOp,
  GreaterThanOp,
  GreaterThanOrEqualOp,
  AndOp,
  OrOp,
  NotOp,
  AddOp,
  SubtractOp,
  MultiplyOp,
  DivideOp,
  UnaryMinusOp,
  AddAssignOp,
  SubtractAssignOp,
  MultiplyAssignOp,
  DivideAssignOp,
  PrefixIncOp,
  PrefixDecOp,
  PostfixIncOp,
  PostfixDecOp,
  IndexedAssign,
  IndexOp,
  IndexedAddAssignOp,
  IndexedSubtractAssignOp,
  IndexedMultiplyAssignOp,
  IndexedDivideAssignOp,
  IndexedPrefixIncOp,
  IndexedPrefixDecOp,
  IndexedPostfixIncOp,
  IndexedPostfixDecOp,
  CreateMatrix,
  CreateArray,


  StartOfUserOpcodes =
      16000  // This OP code always need to have the highest value - preferably constant.
};

}  // namespace vm
}  // namespace fetch
