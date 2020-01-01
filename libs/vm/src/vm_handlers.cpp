//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "vm/array.hpp"
#include "vm/estimate_charge.hpp"
#include "vm/fixed.hpp"
#include "vm/vm.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace fetch {
namespace vm {

void VM::Handler__LocalVariableDeclare()
{
  Variant &variable = GetLocalVariable(instruction_->index);
  if (instruction_->type_id > TypeIds::PrimitiveMaxId)
  {
    variable.Construct(Ptr<Object>(), instruction_->type_id);
    live_object_stack_.emplace_back(frame_sp_, instruction_->index, instruction_->data);
  }
  else
  {
    Primitive p{0u};
    variable.Construct(p, instruction_->type_id);
  }
}

void VM::Handler__LocalVariableDeclareAssign()
{
  Variant &variable = GetLocalVariable(instruction_->index);
  variable          = std::move(Pop());
  if (instruction_->type_id > TypeIds::PrimitiveMaxId)
  {
    live_object_stack_.emplace_back(frame_sp_, instruction_->index, instruction_->data);
  }
}

void VM::Handler__PushNull()
{
  if (++sp_ < STACK_SIZE)
  {
    Top().Construct(Ptr<Object>(), instruction_->type_id);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__PushFalse()
{
  if (++sp_ < STACK_SIZE)
  {
    Top().Construct(false, TypeIds::Bool);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__PushTrue()
{
  if (++sp_ < STACK_SIZE)
  {
    Top().Construct(true, TypeIds::Bool);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__PushString()
{
  if (++sp_ < STACK_SIZE)
  {
    Top().Construct(strings_[instruction_->index], TypeIds::String);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__PushConstant()
{
  if (++sp_ < STACK_SIZE)
  {
    Variant const &constant = executable_->constants[instruction_->index];
    Top().Construct(constant);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__PushLocalVariable()
{
  if (++sp_ < STACK_SIZE)
  {
    Variant const &variable = GetLocalVariable(instruction_->index);
    Top().Construct(variable);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__PopToLocalVariable()
{
  Variant &variable = GetLocalVariable(instruction_->index);
  variable          = std::move(Pop());
}

void VM::Handler__Inc()
{
  Variant &top = Top();
  ExecuteIntegralOp<Inc>(instruction_->type_id, top, top);
}

void VM::Handler__Dec()
{
  Variant &top = Top();
  ExecuteIntegralOp<Dec>(instruction_->type_id, top, top);
}

void VM::Handler__Duplicate()
{
  if (sp_ + instruction_->data < STACK_SIZE)
  {
    int end = sp_;
    int p   = sp_ - instruction_->data;
    do
    {
      stack_[++sp_].Construct(stack_[++p]);
    } while (p < end);
    return;
  }
  RuntimeError("stack overflow");
}

void VM::Handler__DuplicateInsert()
{
  if (sp_ + 1 < STACK_SIZE)
  {
    int end = sp_ - instruction_->data;
    for (int p = sp_; p >= end; --p)
    {
      stack_[p + 1] = std::move(stack_[p]);
    }
    stack_[end] = stack_[++sp_];
    return;
  }
  RuntimeError("stack overflow");
}

void VM::Handler__Discard()
{
  Pop().Reset();
}

void VM::Handler__Destruct()
{
  Destruct(instruction_->data);
}

void VM::Handler__Break()
{
  Destruct(instruction_->data);
  pc_ = instruction_->index;
}

void VM::Handler__Continue()
{
  Destruct(instruction_->data);
  pc_ = instruction_->index;
}

void VM::Handler__Jump()
{
  pc_ = instruction_->index;
}

void VM::Handler__JumpIfFalse()
{
  Variant &v = Pop();
  if (v.primitive.ui8 == 0)
  {
    pc_ = instruction_->index;
  }
  v.Reset();
}

void VM::Handler__JumpIfTrue()
{
  Variant &v = Pop();
  if (v.primitive.ui8 != 0)
  {
    pc_ = instruction_->index;
  }
  v.Reset();
}

// NOTE: Opcodes::Return and Opcodes::ReturnValue both route through here
void VM::Handler__Return()
{
  Destruct(0);
  if (function_->kind == FunctionKind::UserDefinedFreeFunction)
  {
    if (instruction_->opcode == Opcodes::ReturnValue)
    {
      // Reset the 2nd and subsequent parameters
      for (int i = bsp_ + 1; i < bsp_ + function_->num_parameters; ++i)
      {
        stack_[i].Reset();
      }
      // Store the return value
      if (sp_ != bsp_)
      {
        stack_[bsp_] = std::move(stack_[sp_]);
      }
      sp_ = bsp_;
    }
    else  // instruction_->opcode == Opcodes::Return
    {
      // Reset all the parameters
      for (int i = bsp_; i < bsp_ + function_->num_parameters; ++i)
      {
        stack_[i].Reset();
      }
      sp_ = bsp_ - 1;
    }
  }
  else if (function_->kind == FunctionKind::UserDefinedMemberFunction)
  {
    if (instruction_->opcode == Opcodes::ReturnValue)
    {
      // Reset all the parameters
      for (int i = bsp_; i < bsp_ + function_->num_parameters; ++i)
      {
        stack_[i].Reset();
      }
      // Store the return value over the top of the invoker
      --bsp_;
      stack_[bsp_] = std::move(stack_[sp_]);
      sp_          = bsp_;
    }
    else  // instruction_->opcode == Opcodes::Return
    {
      // Reset the invoker and all the parameters
      for (int i = bsp_ - 1; i < bsp_ + function_->num_parameters; ++i)
      {
        stack_[i].Reset();
      }
      sp_ = bsp_ - 2;
    }
  }
  else
  {
    // function_->kind == FunctionKind::UserDefinedConstructor
    // Reset the 2nd and subsequent parameters
    for (int i = bsp_ + 1; i < bsp_ + function_->num_parameters; ++i)
    {
      stack_[i].Reset();
    }
    // Store the constructed object
    if (bsp_ < STACK_SIZE)
    {
      stack_[bsp_] = std::move(self_);
      sp_          = bsp_;
      return;
    }
    RuntimeError("stack overflow");
    return;
  }

  if (frame_sp_ != -1)
  {
    // We've finished executing an inner function
    PopFrame();
  }
  else
  {
    // We've finished executing the outermost free function
    stop_ = true;
  }
}

void VM::Handler__ForRangeInit()
{
  ForRangeLoop loop{};
  loop.variable_index = instruction_->index;
  Variant &variable   = GetLocalVariable(loop.variable_index);
  variable.type_id    = instruction_->type_id;
  if (instruction_->data == 2)
  {
    Variant &targetv = Pop();
    Variant &startv  = Pop();
    loop.current     = startv.primitive;
    loop.target      = targetv.primitive;
    targetv.Reset();
    startv.Reset();
  }
  else
  {
    Variant &deltav  = Pop();
    Variant &targetv = Pop();
    Variant &startv  = Pop();
    loop.current     = startv.primitive;
    loop.target      = targetv.primitive;
    loop.delta       = deltav.primitive;
    deltav.Reset();
    targetv.Reset();
    startv.Reset();
  }
  if (++range_loop_sp_ < MAX_RANGE_LOOPS)
  {
    range_loop_stack_[range_loop_sp_] = loop;
    return;
  }
  --range_loop_sp_;
  RuntimeError("for stack overflow");
}

void VM::Handler__ForRangeIterate()
{
  ForRangeLoop &loop     = range_loop_stack_[range_loop_sp_];
  Variant &     variable = GetLocalVariable(loop.variable_index);
  bool          finished = true;
  if (instruction_->data == 2)
  {
    switch (variable.type_id)
    {
    case TypeIds::Int8:
    {
      variable.primitive.i8 = loop.current.i8++;
      finished              = variable.primitive.i8 >= loop.target.i8;
      break;
    }
    case TypeIds::UInt8:
    {
      variable.primitive.ui8 = loop.current.ui8++;
      finished               = variable.primitive.ui8 >= loop.target.ui8;
      break;
    }
    case TypeIds::Int16:
    {
      variable.primitive.i16 = loop.current.i16++;
      finished               = variable.primitive.i16 >= loop.target.i16;
      break;
    }
    case TypeIds::UInt16:
    {
      variable.primitive.ui16 = loop.current.ui16++;
      finished                = variable.primitive.ui16 >= loop.target.ui16;
      break;
    }
    case TypeIds::Int32:
    {
      variable.primitive.i32 = loop.current.i32++;
      finished               = variable.primitive.i32 >= loop.target.i32;
      break;
    }
    case TypeIds::UInt32:
    {
      variable.primitive.ui32 = loop.current.ui32++;
      finished                = variable.primitive.ui32 >= loop.target.ui32;
      break;
    }
    case TypeIds::Int64:
    {
      variable.primitive.i64 = loop.current.i64++;
      finished               = variable.primitive.i64 >= loop.target.i64;
      break;
    }
    case TypeIds::UInt64:
    {
      variable.primitive.ui64 = loop.current.ui64++;
      finished                = variable.primitive.ui64 >= loop.target.ui64;
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }
  else
  {
    switch (variable.type_id)
    {
    case TypeIds::Int8:
    {
      variable.primitive.i8 = loop.current.i8;
      loop.current.i8       = int8_t(loop.current.i8 + loop.delta.i8);
      finished              = variable.primitive.i8 >= loop.target.i8;
      break;
    }
    case TypeIds::UInt8:
    {
      variable.primitive.ui8 = loop.current.ui8;
      loop.current.ui8       = uint8_t(loop.current.ui8 + loop.delta.ui8);
      finished               = variable.primitive.ui8 >= loop.target.ui8;
      break;
    }
    case TypeIds::Int16:
    {
      variable.primitive.i16 = loop.current.i16;
      loop.current.i16       = int16_t(loop.current.i16 + loop.delta.i16);
      finished               = variable.primitive.i16 >= loop.target.i16;
      break;
    }
    case TypeIds::UInt16:
    {
      variable.primitive.ui16 = loop.current.ui16;
      loop.current.ui16       = uint16_t(loop.current.ui16 + loop.delta.ui16);
      finished                = variable.primitive.ui16 >= loop.target.ui16;

      break;
    }
    case TypeIds::Int32:
    {
      variable.primitive.i32 = loop.current.i32;
      loop.current.i32 += loop.delta.i32;
      finished = variable.primitive.i32 >= loop.target.i32;
      break;
    }
    case TypeIds::UInt32:
    {
      variable.primitive.ui32 = loop.current.ui32;
      loop.current.ui32 += loop.delta.ui32;
      finished = variable.primitive.ui32 >= loop.target.ui32;
      break;
    }
    case TypeIds::Int64:
    {
      variable.primitive.i64 = loop.current.i64;
      loop.current.i64 += loop.delta.i64;
      finished = variable.primitive.i64 >= loop.target.i64;
      break;
    }
    case TypeIds::UInt64:
    {
      variable.primitive.ui64 = loop.current.ui64;
      loop.current.ui64 += loop.delta.ui64;
      finished = variable.primitive.ui64 >= loop.target.ui64;
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }
  if (finished)
  {
    pc_ = instruction_->index;
  }
}

void VM::Handler__ForRangeTerminate()
{
  --range_loop_sp_;
}

void VM::Handler__InvokeUserDefinedFreeFunction()
{
  if (!PushFrame())
  {
    return;
  }

  function_ = &(executable_->functions[instruction_->index]);
  bsp_      = sp_ - function_->num_parameters + 1;  // first parameter
  pc_       = 0;
  self_.Reset();

  int const num_locals = function_->num_variables - function_->num_parameters;
  sp_ += num_locals;
  if (sp_ < STACK_SIZE)
  {
    return;
  }
  sp_ -= num_locals;
  RuntimeError("stack overflow");
}

void VM::Handler__LocalVariablePrefixInc()
{
  DoLocalVariablePrefixPostfixOp<PrefixInc>();
}

void VM::Handler__LocalVariablePrefixDec()
{
  DoLocalVariablePrefixPostfixOp<PrefixDec>();
}

void VM::Handler__LocalVariablePostfixInc()
{
  DoLocalVariablePrefixPostfixOp<PostfixInc>();
}

void VM::Handler__LocalVariablePostfixDec()
{
  DoLocalVariablePrefixPostfixOp<PostfixDec>();
}

void VM::Handler__JumpIfFalseOrPop()
{
  Variant &v = Top();
  if (v.primitive.ui8 == 0)
  {
    pc_ = instruction_->index;
  }
  else
  {
    v.Reset();
    --sp_;
  }
}

void VM::Handler__JumpIfTrueOrPop()
{
  Variant &v = Top();
  if (v.primitive.ui8 != 0)
  {
    pc_ = instruction_->index;
  }
  else
  {
    v.Reset();
    --sp_;
  }
}

void VM::Handler__Not()
{
  Variant &top      = Top();
  top.primitive.ui8 = top.primitive.ui8 == 0 ? 1 : 0;
}

void VM::Handler__PrimitiveEqual()
{
  DoPrimitiveRelationalOp<PrimitiveEqual>();
}

void VM::Handler__ObjectEqual()
{
  Variant &rhsv = Pop();
  Variant &lhsv = Top();
  lhsv.Assign(IsEqual(lhsv.object, rhsv.object), TypeIds::Bool);
  rhsv.Reset();
}

void VM::Handler__PrimitiveNotEqual()
{
  DoPrimitiveRelationalOp<PrimitiveNotEqual>();
}

void VM::Handler__ObjectNotEqual()
{
  Variant &rhsv = Pop();
  Variant &lhsv = Top();
  lhsv.Assign(IsNotEqual(lhsv.object, rhsv.object), TypeIds::Bool);
  rhsv.Reset();
}

void VM::Handler__PrimitiveLessThan()
{
  DoPrimitiveRelationalOp<PrimitiveLessThan>();
}

void VM::Handler__ObjectLessThan()
{
  DoObjectRelationalOp<ObjectLessThan>();
}

void VM::Handler__PrimitiveLessThanOrEqual()
{
  DoPrimitiveRelationalOp<PrimitiveLessThanOrEqual>();
}

void VM::Handler__ObjectLessThanOrEqual()
{
  DoObjectRelationalOp<ObjectLessThanOrEqual>();
}

void VM::Handler__PrimitiveGreaterThan()
{
  DoPrimitiveRelationalOp<PrimitiveGreaterThan>();
}

void VM::Handler__ObjectGreaterThan()
{
  DoObjectRelationalOp<ObjectGreaterThan>();
}

void VM::Handler__PrimitiveGreaterThanOrEqual()
{
  DoPrimitiveRelationalOp<PrimitiveGreaterThanOrEqual>();
}

void VM::Handler__ObjectGreaterThanOrEqual()
{
  DoObjectRelationalOp<ObjectGreaterThanOrEqual>();
}

void VM::Handler__PrimitiveNegate()
{
  Variant &top = Top();
  ExecuteNumericOp<PrimitiveNegate>(instruction_->type_id, top, top);
}

void VM::Handler__ObjectNegate()
{
  Variant &top = Top();
  if (top.object)
  {
    if (EstimateCharge(this, ChargeEstimator<>([top]() -> ChargeAmount {
                         return top.object->NegateChargeEstimator(top.object);
                       }),
                       std::tuple<>{}))
    {
      top.object->Negate(top.object);
    }
    return;
  }
  RuntimeError("null reference");
}

void VM::Handler__PrimitiveAdd()
{
  DoNumericOp<PrimitiveAdd>();
}

void VM::Handler__ObjectAdd()
{
  DoObjectOp<ObjectAdd>();
}

void VM::Handler__ObjectLeftAdd()
{
  DoObjectLeftOp<ObjectLeftAdd>();
}

void VM::Handler__ObjectRightAdd()
{
  DoObjectRightOp<ObjectRightAdd>();
}

void VM::Handler__LocalVariablePrimitiveInplaceAdd()
{
  DoLocalVariableNumericInplaceOp<PrimitiveAdd>();
}

void VM::Handler__LocalVariableObjectInplaceAdd()
{
  DoLocalVariableObjectInplaceOp<ObjectInplaceAdd>();
}

void VM::Handler__LocalVariableObjectInplaceRightAdd()
{
  DoLocalVariableObjectInplaceRightOp<ObjectInplaceRightAdd>();
}

void VM::Handler__PrimitiveSubtract()
{
  DoNumericOp<PrimitiveSubtract>();
}

void VM::Handler__ObjectSubtract()
{
  DoObjectOp<ObjectSubtract>();
}

void VM::Handler__ObjectLeftSubtract()
{
  DoObjectLeftOp<ObjectLeftSubtract>();
}

void VM::Handler__ObjectRightSubtract()
{
  DoObjectRightOp<ObjectRightSubtract>();
}

void VM::Handler__LocalVariablePrimitiveInplaceSubtract()
{
  DoLocalVariableNumericInplaceOp<PrimitiveSubtract>();
}

void VM::Handler__LocalVariableObjectInplaceSubtract()
{
  DoLocalVariableObjectInplaceOp<ObjectInplaceSubtract>();
}

void VM::Handler__LocalVariableObjectInplaceRightSubtract()
{
  DoLocalVariableObjectInplaceRightOp<ObjectInplaceRightSubtract>();
}

void VM::Handler__PrimitiveMultiply()
{
  DoNumericOp<PrimitiveMultiply>();
}

void VM::Handler__ObjectMultiply()
{
  DoObjectOp<ObjectMultiply>();
}

void VM::Handler__ObjectLeftMultiply()
{
  DoObjectLeftOp<ObjectLeftMultiply>();
}

void VM::Handler__ObjectRightMultiply()
{
  DoObjectRightOp<ObjectRightMultiply>();
}

void VM::Handler__LocalVariablePrimitiveInplaceMultiply()
{
  DoLocalVariableNumericInplaceOp<PrimitiveMultiply>();
}

void VM::Handler__LocalVariableObjectInplaceMultiply()
{
  DoLocalVariableObjectInplaceOp<ObjectInplaceMultiply>();
}

void VM::Handler__LocalVariableObjectInplaceRightMultiply()
{
  DoLocalVariableObjectInplaceRightOp<ObjectInplaceRightMultiply>();
}

void VM::Handler__PrimitiveDivide()
{
  DoNumericOp<PrimitiveDivide>();
}

void VM::Handler__ObjectDivide()
{
  DoObjectOp<ObjectDivide>();
}

void VM::Handler__ObjectLeftDivide()
{
  DoObjectLeftOp<ObjectLeftDivide>();
}

void VM::Handler__ObjectRightDivide()
{
  DoObjectRightOp<ObjectRightDivide>();
}

void VM::Handler__LocalVariablePrimitiveInplaceDivide()
{
  DoLocalVariableNumericInplaceOp<PrimitiveDivide>();
}

void VM::Handler__LocalVariableObjectInplaceDivide()
{
  DoLocalVariableObjectInplaceOp<ObjectInplaceDivide>();
}

void VM::Handler__LocalVariableObjectInplaceRightDivide()
{
  DoLocalVariableObjectInplaceRightOp<ObjectInplaceRightDivide>();
}

void VM::Handler__PrimitiveModulo()
{
  DoIntegralOp<PrimitiveModulo>();
}

void VM::Handler__LocalVariablePrimitiveInplaceModulo()
{
  DoLocalVariableIntegralInplaceOp<PrimitiveModulo>();
}

void VM::Handler__InitialiseArray()
{
  auto seq_size{instruction_->data};

  auto               ret_val{IArray::Constructor(this, instruction_->type_id, seq_size)};
  TemplateParameter1 element;
  for (AnyInteger i(seq_size, TypeIds::UInt16); i.primitive.ui16 > 0;)
  {
    --i.primitive.ui16;
    element.Construct(Pop());
    ret_val->SetIndexedValue(i, element);
  }

  if (++sp_ < STACK_SIZE)
  {
    Top().Construct(ret_val, instruction_->type_id);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__ContractVariableDeclareAssign()
{
  // The contract id is stored in instruction_->type_id
  Variant sv = std::move(Pop());
  assert(sv.type_id == TypeIds::String);
  if (!sv.object)
  {
    RuntimeError("null reference");
    return;
  }
  std::string identity = Ptr<String>(sv.object)->string();
  // Clone the identity string
  sv.object         = Ptr<String>(new String(this, identity));
  Variant &variable = GetLocalVariable(instruction_->index);
  variable          = std::move(sv);
  live_object_stack_.emplace_back(frame_sp_, instruction_->index, instruction_->data);
}

void VM::Handler__InvokeContractFunction()
{
  uint16_t                    contract_id = instruction_->data;
  uint16_t                    function_id = instruction_->index;
  Executable::Contract const &contract    = executable_->contracts[contract_id];
  Executable::Function const &function    = contract.functions[function_id];
  VariantArray                parameters(std::size_t(function.num_parameters));
  int                         count = function.num_parameters;
  while (--count >= 0)
  {
    parameters[std::size_t(count)] = std::move(Pop());
  }
  Variant &   sv       = Pop();
  std::string identity = Ptr<String>(sv.object)->string();
  sv.Reset();

  if (!contract_invocation_handler_)
  {
    RuntimeError("Contract-to-contract calls not supported: invocation handler is null");
    return;
  }

  std::string error;
  Variant     output;

  bool ok =
      contract_invocation_handler_(this, identity, contract, function, parameters, error, output);

  if (!ok)
  {
    RuntimeError(error);
    return;
  }

  if (function.return_type_id != TypeIds::Void)
  {
    if (output.type_id != function.return_type_id)
    {
      RuntimeError("Call to " + function.name + " in contract " + identity +
                   " returned unexpected type_id");
      return;
    }
    if (++sp_ < STACK_SIZE)
    {
      Variant &top = Top();
      top          = std::move(output);
      return;
    }
    --sp_;
    RuntimeError("stack overflow");
  }
}

void VM::Handler__PushLargeConstant()
{
  if (++sp_ < STACK_SIZE)
  {
    Executable::LargeConstant const &constant = executable_->large_constants[instruction_->index];
    assert(constant.type_id == TypeIds::Fixed128);
    auto object = Ptr<Fixed128>(new Fixed128(this, constant.fp128));
    Top().Construct(object, TypeIds::Fixed128);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__PushMemberVariable()
{
  Variant &              objectv             = Top();
  Ptr<UserDefinedObject> user_defined_object = std::move(objectv.object);
  if (user_defined_object)
  {
    Variant &variable = user_defined_object->GetVariable(instruction_->index);
    objectv           = variable;
    return;
  }
  RuntimeError("null reference");
}

void VM::Handler__PopToMemberVariable()
{
  Variant &              rhsv                = Pop();
  Variant &              objectv             = Pop();
  Ptr<UserDefinedObject> user_defined_object = std::move(objectv.object);
  if (user_defined_object)
  {
    Variant &variable = user_defined_object->GetVariable(instruction_->index);
    variable          = std::move(rhsv);
    objectv.Reset();
    return;
  }
  RuntimeError("null reference");
}

void VM::Handler__MemberVariablePrefixInc()
{
  DoMemberVariablePrefixPostfixOp<PrefixInc>();
}

void VM::Handler__MemberVariablePrefixDec()
{
  DoMemberVariablePrefixPostfixOp<PrefixDec>();
}

void VM::Handler__MemberVariablePostfixInc()
{
  DoMemberVariablePrefixPostfixOp<PostfixInc>();
}

void VM::Handler__MemberVariablePostfixDec()
{
  DoMemberVariablePrefixPostfixOp<PostfixDec>();
}

void VM::Handler__MemberVariablePrimitiveInplaceAdd()
{
  DoMemberVariableNumericInplaceOp<PrimitiveAdd>();
}

void VM::Handler__MemberVariableObjectInplaceAdd()
{
  DoMemberVariableObjectInplaceOp<ObjectInplaceAdd>();
}

void VM::Handler__MemberVariableObjectInplaceRightAdd()
{
  DoMemberVariableObjectInplaceRightOp<ObjectInplaceRightAdd>();
}

void VM::Handler__MemberVariablePrimitiveInplaceSubtract()
{
  DoMemberVariableNumericInplaceOp<PrimitiveSubtract>();
}

void VM::Handler__MemberVariableObjectInplaceSubtract()
{
  DoMemberVariableObjectInplaceOp<ObjectInplaceSubtract>();
}

void VM::Handler__MemberVariableObjectInplaceRightSubtract()
{
  DoMemberVariableObjectInplaceRightOp<ObjectInplaceRightSubtract>();
}

void VM::Handler__MemberVariablePrimitiveInplaceMultiply()
{
  DoMemberVariableNumericInplaceOp<PrimitiveMultiply>();
}

void VM::Handler__MemberVariableObjectInplaceMultiply()
{
  DoMemberVariableObjectInplaceOp<ObjectInplaceMultiply>();
}

void VM::Handler__MemberVariableObjectInplaceRightMultiply()
{
  DoMemberVariableObjectInplaceRightOp<ObjectInplaceRightMultiply>();
}

void VM::Handler__MemberVariablePrimitiveInplaceDivide()
{
  DoMemberVariableNumericInplaceOp<PrimitiveDivide>();
}

void VM::Handler__MemberVariableObjectInplaceDivide()
{
  DoMemberVariableObjectInplaceOp<ObjectInplaceDivide>();
}

void VM::Handler__MemberVariableObjectInplaceRightDivide()
{
  DoMemberVariableObjectInplaceRightOp<ObjectInplaceRightDivide>();
}

void VM::Handler__MemberVariablePrimitiveInplaceModulo()
{
  DoMemberVariableIntegralInplaceOp<PrimitiveModulo>();
}

void VM::Handler__PushSelf()
{
  if (++sp_ < STACK_SIZE)
  {
    Top().Construct(self_);
    return;
  }
  --sp_;
  RuntimeError("stack overflow");
}

void VM::Handler__InvokeUserDefinedConstructor()
{
  TypeId  type_id = instruction_->type_id;
  Variant selfv(Ptr<UserDefinedObject>(new UserDefinedObject(this, type_id)), type_id);
  Executable::UserDefinedType const &user_defined_type = GetUserDefinedType(type_id);
  Executable::Function const *constructor = &(user_defined_type.functions[instruction_->index]);
  if (constructor->instructions.empty())
  {
    // System-supplied default constructor means no user code to run, so just push the new object
    if (++sp_ < STACK_SIZE)
    {
      Top().Construct(std::move(selfv));
      return;
    }
    --sp_;
    RuntimeError("stack overflow");
    return;
  }

  if (!PushFrame())
  {
    return;
  }

  function_ = constructor;
  bsp_      = sp_ - function_->num_parameters + 1;  // first parameter
  pc_       = 0;
  self_     = std::move(selfv);

  int const num_locals = function_->num_variables - function_->num_parameters;
  sp_ += num_locals;
  if (sp_ < STACK_SIZE)
  {
    return;
  }
  sp_ -= num_locals;
  RuntimeError("stack overflow");
}

void VM::Handler__InvokeUserDefinedMemberFunction()
{
  if (!PushFrame())
  {
    return;
  }

  TypeId                             invoker_type_id   = instruction_->data;
  Executable::UserDefinedType const &user_defined_type = GetUserDefinedType(invoker_type_id);

  function_ = &(user_defined_type.functions[instruction_->index]);
  bsp_      = sp_ - function_->num_parameters + 1;  // first parameter
  pc_       = 0;
  self_     = std::move(stack_[bsp_ - 1]);  // the invoker

  if (self_.object)
  {
    int const num_locals = function_->num_variables - function_->num_parameters;
    sp_ += num_locals;
    if (sp_ < STACK_SIZE)
    {
      return;
    }
    sp_ -= num_locals;
    RuntimeError("stack overflow");
    return;
  }
  RuntimeError("null reference");
}

}  // namespace vm
}  // namespace fetch
