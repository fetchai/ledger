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

namespace fetch {
namespace vm {

namespace Opcodes {
static constexpr uint16_t Unknown                            = 0;
static constexpr uint16_t VariableDeclare                    = 1;
static constexpr uint16_t VariableDeclareAssign              = 2;
static constexpr uint16_t PushNull                           = 3;
static constexpr uint16_t PushFalse                          = 4;
static constexpr uint16_t PushTrue                           = 5;
static constexpr uint16_t PushString                         = 6;
static constexpr uint16_t PushConstant                       = 7;
static constexpr uint16_t PushVariable                       = 8;
static constexpr uint16_t PopToVariable                      = 9;
static constexpr uint16_t Inc                                = 10;
static constexpr uint16_t Dec                                = 11;
static constexpr uint16_t Duplicate                          = 12;
static constexpr uint16_t DuplicateInsert                    = 13;
static constexpr uint16_t Discard                            = 14;
static constexpr uint16_t Destruct                           = 15;
static constexpr uint16_t Break                              = 16;
static constexpr uint16_t Continue                           = 17;
static constexpr uint16_t Jump                               = 18;
static constexpr uint16_t JumpIfFalse                        = 19;
static constexpr uint16_t JumpIfTrue                         = 20;
static constexpr uint16_t Return                             = 21;
static constexpr uint16_t ReturnValue                        = 22;
static constexpr uint16_t ForRangeInit                       = 23;
static constexpr uint16_t ForRangeIterate                    = 24;
static constexpr uint16_t ForRangeTerminate                  = 25;
static constexpr uint16_t InvokeUserDefinedFreeFunction      = 26;
static constexpr uint16_t VariablePrefixInc                  = 27;
static constexpr uint16_t VariablePrefixDec                  = 28;
static constexpr uint16_t VariablePostfixInc                 = 29;
static constexpr uint16_t VariablePostfixDec                 = 30;
static constexpr uint16_t JumpIfFalseOrPop                   = 31;
static constexpr uint16_t JumpIfTrueOrPop                    = 32;
static constexpr uint16_t Not                                = 33;
static constexpr uint16_t PrimitiveEqual                     = 34;
static constexpr uint16_t ObjectEqual                        = 35;
static constexpr uint16_t PrimitiveNotEqual                  = 36;
static constexpr uint16_t ObjectNotEqual                     = 37;
static constexpr uint16_t PrimitiveLessThan                  = 38;
static constexpr uint16_t ObjectLessThan                     = 39;
static constexpr uint16_t PrimitiveLessThanOrEqual           = 40;
static constexpr uint16_t ObjectLessThanOrEqual              = 41;
static constexpr uint16_t PrimitiveGreaterThan               = 42;
static constexpr uint16_t ObjectGreaterThan                  = 43;
static constexpr uint16_t PrimitiveGreaterThanOrEqual        = 44;
static constexpr uint16_t ObjectGreaterThanOrEqual           = 45;
static constexpr uint16_t PrimitiveNegate                    = 46;
static constexpr uint16_t ObjectNegate                       = 47;
static constexpr uint16_t PrimitiveAdd                       = 48;
static constexpr uint16_t ObjectAdd                          = 49;
static constexpr uint16_t ObjectLeftAdd                      = 50;
static constexpr uint16_t ObjectRightAdd                     = 51;
static constexpr uint16_t VariablePrimitiveInplaceAdd        = 52;
static constexpr uint16_t VariableObjectInplaceAdd           = 53;
static constexpr uint16_t VariableObjectInplaceRightAdd      = 54;
static constexpr uint16_t PrimitiveSubtract                  = 55;
static constexpr uint16_t ObjectSubtract                     = 56;
static constexpr uint16_t ObjectLeftSubtract                 = 57;
static constexpr uint16_t ObjectRightSubtract                = 58;
static constexpr uint16_t VariablePrimitiveInplaceSubtract   = 59;
static constexpr uint16_t VariableObjectInplaceSubtract      = 60;
static constexpr uint16_t VariableObjectInplaceRightSubtract = 61;
static constexpr uint16_t PrimitiveMultiply                  = 62;
static constexpr uint16_t ObjectMultiply                     = 63;
static constexpr uint16_t ObjectLeftMultiply                 = 64;
static constexpr uint16_t ObjectRightMultiply                = 65;
static constexpr uint16_t VariablePrimitiveInplaceMultiply   = 66;
static constexpr uint16_t VariableObjectInplaceMultiply      = 67;
static constexpr uint16_t VariableObjectInplaceRightMultiply = 68;
static constexpr uint16_t PrimitiveDivide                    = 69;
static constexpr uint16_t ObjectDivide                       = 70;
static constexpr uint16_t ObjectLeftDivide                   = 71;
static constexpr uint16_t ObjectRightDivide                  = 72;
static constexpr uint16_t VariablePrimitiveInplaceDivide     = 73;
static constexpr uint16_t VariableObjectInplaceDivide        = 74;
static constexpr uint16_t VariableObjectInplaceRightDivide   = 75;
static constexpr uint16_t PrimitiveModulo                    = 76;
static constexpr uint16_t VariablePrimitiveInplaceModulo     = 77;
static constexpr uint16_t InitializeArray                    = 78;
static constexpr uint16_t NumReserved                        = 79;
}  // namespace Opcodes

}  // namespace vm
}  // namespace fetch
