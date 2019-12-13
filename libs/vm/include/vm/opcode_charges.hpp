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
namespace vm {

namespace OpcodeCharges {

static const ChargeAmount DEFAULT_OBJECT_CHARGE = 1000;
static const ChargeAmount DEFAULT_STATIC_CHARGE = 100;

static const ChargeAmount CHARGE_LOCAL_VARIABLE_DECLARE                    = 9;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_DECLARE_ASSIGN             = 1;
static const ChargeAmount CHARGE_PUSH_NULL                                 = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_PUSH_FALSE                                = 15;
static const ChargeAmount CHARGE_PUSH_TRUE                                 = 4;
static const ChargeAmount CHARGE_PUSH_STRING                               = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PUSH_CONSTANT                             = 14;
static const ChargeAmount CHARGE_PUSH_LOCAL_VARIABLE                       = 2;
static const ChargeAmount CHARGE_POP_TO_LOCAL_VARIABLE                     = 8;
static const ChargeAmount CHARGE_INC                                       = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_DEC                                       = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_DUPLICATE                                 = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_DUPLICATE_INSERT                          = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_DISCARD                                   = 8;
static const ChargeAmount CHARGE_DESTRUCT                                  = 3;
static const ChargeAmount CHARGE_BREAK                                     = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_CONTINUE                                  = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_JUMP                                      = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_JUMP_IF_FALSE                             = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_JUMP_IF_TRUE                              = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_RETURN                                    = 32;
static const ChargeAmount CHARGE_RETURN_VALUE                              = 26;
static const ChargeAmount CHARGE_FOR_RANGE_INIT                            = 7;
static const ChargeAmount CHARGE_FOR_RANGE_ITERATE                         = 7;
static const ChargeAmount CHARGE_FOR_RANGE_TERMINATE                       = 7;
static const ChargeAmount CHARGE_INVOKE_USER_DEFINED_FREE_FUNCTION         = 19;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_PREFIX_INC                 = 3;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_PREFIX_DEC                 = 3;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_POSTFIX_INC                = 3;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_POSTFIX_DEC                = 3;
static const ChargeAmount CHARGE_JUMP_IF_FALSE_OR_POP                      = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_JUMP_IF_TRUE_OR_POP                       = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_NOT                                       = 5;
static const ChargeAmount CHARGE_PRIMITIVE_EQUAL                           = 13;
static const ChargeAmount CHARGE_OBJECT_EQUAL                              = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_NOT_EQUAL                       = 13;
static const ChargeAmount CHARGE_OBJECT_NOT_EQUAL                          = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_LESS_THAN                       = 13;
static const ChargeAmount CHARGE_OBJECT_LESS_THAN                          = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_LESS_THAN_OR_EQUAL              = 14;
static const ChargeAmount CHARGE_OBJECT_LESS_THAN_OR_EQUAL                 = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_GREATER_THAN                    = 14;
static const ChargeAmount CHARGE_OBJECT_GREATER_THAN                       = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_GREATER_THAN_OR_EQUAL           = 14;
static const ChargeAmount CHARGE_OBJECT_GREATER_THAN_OR_EQUAL              = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_NEGATE                          = 5;
static const ChargeAmount CHARGE_OBJECT_NEGATE                             = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_ADD                             = 13;
static const ChargeAmount CHARGE_OBJECT_ADD                                = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_OBJECT_LEFT_ADD                           = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_OBJECT_RIGHT_ADD                          = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_PRIMITIVE_INPLACE_ADD      = 12;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_OBJECT_INPLACE_ADD         = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_OBJECT_INPLACE_RIGHT_ADD   = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_SUBTRACT                        = 13;
static const ChargeAmount CHARGE_OBJECT_SUBTRACT                           = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_OBJECT_LEFT_SUBTRACT                      = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_OBJECT_RIGHT_SUBTRACT                     = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_PRIMITIVE_INPLACE_SUBTRACT = 12;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_OBJECT_INPLACE_SUBTRACT    = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_OBJECT_INPLACE_RIGHT_SUBTRACT =
    DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_MULTIPLY                        = 14;
static const ChargeAmount CHARGE_OBJECT_MULTIPLY                           = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_OBJECT_LEFT_MULTIPLY                      = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_OBJECT_RIGHT_MULTIPLY                     = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_PRIMITIVE_INPLACE_MULTIPLY = 14;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_OBJECT_INPLACE_MULTIPLY    = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_OBJECT_INPLACE_RIGHT_MULTIPLY =
    DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_DIVIDE                           = 31;
static const ChargeAmount CHARGE_OBJECT_DIVIDE                              = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_OBJECT_LEFT_DIVIDE                         = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_OBJECT_RIGHT_DIVIDE                        = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_PRIMITIVE_INPLACE_DIVIDE    = 31;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_OBJECT_INPLACE_DIVIDE       = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_OBJECT_INPLACE_RIGHT_DIVIDE = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_PRIMITIVE_MODULO                           = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_LOCAL_VARIABLE_PRIMITIVE_INPLACE_MODULO    = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_INITIALISE_ARRAY                           = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_CONTRACT_VARIABLE_DECLARE_ASSIGN           = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_INVOKE_CONTRACT_FUNCTION                   = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_PUSH_LARGE_CONSTANT                        = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_PUSH_MEMBER_VARIABLE                       = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_POP_TO_MEMBER_VARIABLE                     = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_PREFIX_INC                 = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_PREFIX_DEC                 = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_POSTFIX_INC                = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_POSTFIX_DEC                = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_PRIMITIVE_INPLACE_ADD      = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_OBJECT_INPLACE_ADD         = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_OBJECT_INPLACE_RIGHT_ADD   = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_PRIMITIVE_INPLACE_SUBTRACT = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_OBJECT_INPLACE_SUBTRACT    = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_OBJECT_INPLACE_RIGHT_SUBTRACT =
    DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_PRIMITIVE_INPLACE_MULTIPLY = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_OBJECT_INPLACE_MULTIPLY    = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_OBJECT_INPLACE_RIGHT_MULTIPLY =
    DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_PRIMITIVE_INPLACE_DIVIDE = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_OBJECT_INPLACE_DIVIDE    = DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_OBJECT_INPLACE_RIGHT_DIVIDE =
    DEFAULT_OBJECT_CHARGE;
static const ChargeAmount CHARGE_MEMBER_VARIABLE_PRIMITIVE_INPLACE_MODULO = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_PUSH_SELF                                = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_INVOKE_USER_DEFINED_CONSTRUCTOR          = DEFAULT_STATIC_CHARGE;
static const ChargeAmount CHARGE_INVOKE_USER_DEFINED_MEMBER_FUNCTION      = DEFAULT_STATIC_CHARGE;
}  // namespace OpcodeCharges

}  // namespace vm
}  // namespace fetch
