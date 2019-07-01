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

#include "vm/vm.hpp"

namespace fetch {
namespace vm {

void VM::Handler__VariableDeclare()
{
  Variant &variable = GetVariable(instruction_->index);
  if (instruction_->type_id > TypeIds::PrimitiveMaxId)
  {
    variable.Construct(Ptr<Object>(), instruction_->type_id);
    LiveObjectInfo &info = live_object_stack_[++live_object_sp_];
    info.frame_sp        = frame_sp_;
    info.variable_index  = instruction_->index;
    info.scope_number    = instruction_->data;
  }
  else
  {
    Primitive p;
    p.Zero();
    variable.Construct(p, instruction_->type_id);
  }
}

void VM::Handler__VariableDeclareAssign()
{
  Variant &variable = GetVariable(instruction_->index);
  variable          = std::move(Pop());
  if (instruction_->type_id > TypeIds::PrimitiveMaxId)
  {
    LiveObjectInfo &info = live_object_stack_[++live_object_sp_];
    info.frame_sp        = frame_sp_;
    info.variable_index  = instruction_->index;
    info.scope_number    = instruction_->data;
  }
}

void VM::Handler__PushNull()
{
  Variant &top = Push();
  top.Construct(Ptr<Object>(), instruction_->type_id);
}

void VM::Handler__PushFalse()
{
  Variant &top = Push();
  top.Construct(false, TypeIds::Bool);
}

void VM::Handler__PushTrue()
{
  Variant &top = Push();
  top.Construct(true, TypeIds::Bool);
}

void VM::Handler__PushString()
{
  Variant &top = Push();
  top.Construct(strings_[instruction_->index], TypeIds::String);
}

void VM::Handler__PushConstant()
{
  Variant &      top      = Push();
  Variant const &constant = executable_->constants[instruction_->index];
  top.Construct(constant);
}

void VM::Handler__PushVariable()
{
  Variant const &variable = GetVariable(instruction_->index);
  Variant &      top      = Push();
  top.Construct(variable);
}

void VM::Handler__PopToVariable()
{
  Variant &variable = GetVariable(instruction_->index);
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
  int end = sp_;
  int p   = sp_ - instruction_->data;
  do
  {
    stack_[++sp_].Construct(stack_[++p]);
  } while (p < end);
}

void VM::Handler__DuplicateInsert()
{
  int end = sp_ - instruction_->data;
  for (int p = sp_; p >= end; --p)
  {
    stack_[p + 1] = std::move(stack_[p]);
  }
  stack_[end] = stack_[++sp_];
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
  if (instruction_->opcode == Opcodes::ReturnValue)
  {
    for (int i = bsp_ + 1; i < bsp_ + function_->num_parameters; ++i)
    {
      stack_[i].Reset();
    }
    if (sp_ != bsp_)
    {
      stack_[bsp_] = std::move(stack_[sp_]);
    }
    sp_ = bsp_;
  }
  else
  {
    for (int i = bsp_; i < bsp_ + function_->num_parameters; ++i)
    {
      stack_[i].Reset();
    }
    sp_ = bsp_ - 1;
  }
  if (frame_sp_ != -1)
  {
    // We've finished executing an inner function
    Frame const &frame = frame_stack_[frame_sp_];
    function_          = frame.function;
    bsp_               = frame.bsp;
    pc_                = frame.pc;
    --frame_sp_;
  }
  else
  {
    // We've finished executing the outermost function
    stop_ = true;
  }
}

void VM::Handler__ForRangeInit()
{
  ForRangeLoop loop;
  loop.variable_index = instruction_->index;
  Variant &variable   = GetVariable(loop.variable_index);
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
  range_loop_stack_[++range_loop_sp_] = loop;
}

void VM::Handler__ForRangeIterate()
{
  ForRangeLoop &loop     = range_loop_stack_[range_loop_sp_];
  Variant &     variable = GetVariable(loop.variable_index);
  bool          finished = true;
  if (instruction_->data == 2)
  {
    switch (variable.type_id)
    {
    case TypeIds::Int8:
    {
      variable.primitive.i8 = loop.current.i8++;
      finished              = variable.primitive.i8 > loop.target.i8;
      break;
    }
    case TypeIds::UInt8:
    {
      variable.primitive.ui8 = loop.current.ui8++;
      finished               = variable.primitive.ui8 > loop.target.ui8;
      break;
    }
    case TypeIds::Int16:
    {
      variable.primitive.i16 = loop.current.i16++;
      finished               = variable.primitive.i16 > loop.target.i16;
      break;
    }
    case TypeIds::UInt16:
    {
      variable.primitive.ui16 = loop.current.ui16++;
      finished                = variable.primitive.ui16 > loop.target.ui16;
      break;
    }
    case TypeIds::Int32:
    {
      variable.primitive.i32 = loop.current.i32++;
      finished               = variable.primitive.i32 > loop.target.i32;
      break;
    }
    case TypeIds::UInt32:
    {
      variable.primitive.ui32 = loop.current.ui32++;
      finished                = variable.primitive.ui32 > loop.target.ui32;
      break;
    }
    case TypeIds::Int64:
    {
      variable.primitive.i64 = loop.current.i64++;
      finished               = variable.primitive.i64 > loop.target.i64;
      break;
    }
    case TypeIds::UInt64:
    {
      variable.primitive.ui64 = loop.current.ui64++;
      finished                = variable.primitive.ui64 > loop.target.ui64;
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
      finished              = (loop.delta.i8 >= 0) ? variable.primitive.i8 > loop.target.i8
                                      : variable.primitive.i8 < loop.target.i8;
      break;
    }
    case TypeIds::UInt8:
    {
      variable.primitive.ui8 = loop.current.ui8;
      loop.current.ui8       = uint8_t(loop.current.ui8 + loop.delta.ui8);
      finished               = variable.primitive.ui8 > loop.target.ui8;
      break;
    }
    case TypeIds::Int16:
    {
      variable.primitive.i16 = loop.current.i16;
      loop.current.i16       = int16_t(loop.current.i16 + loop.delta.i16);
      finished               = (loop.delta.i16 >= 0) ? variable.primitive.i16 > loop.target.i16
                                       : variable.primitive.i16 < loop.target.i16;
      break;
    }
    case TypeIds::UInt16:
    {
      variable.primitive.ui16 = loop.current.ui16;
      loop.current.ui16       = uint16_t(loop.current.ui16 + loop.delta.ui16);
      finished                = variable.primitive.ui16 > loop.target.ui16;

      break;
    }
    case TypeIds::Int32:
    {
      variable.primitive.i32 = loop.current.i32;
      loop.current.i32 += loop.delta.i32;
      finished = (loop.delta.i32 >= 0) ? variable.primitive.i32 > loop.target.i32
                                       : variable.primitive.i32 < loop.target.i32;
      break;
    }
    case TypeIds::UInt32:
    {
      variable.primitive.ui32 = loop.current.ui32;
      loop.current.ui32 += loop.delta.ui32;
      finished = variable.primitive.ui32 > loop.target.ui32;
      break;
    }
    case TypeIds::Int64:
    {
      variable.primitive.i64 = loop.current.i64;
      loop.current.i64 += loop.delta.i64;
      finished = (loop.delta.i64 >= 0) ? variable.primitive.i64 > loop.target.i64
                                       : variable.primitive.i64 < loop.target.i64;
      break;
    }
    case TypeIds::UInt64:
    {
      variable.primitive.ui64 = loop.current.ui64;
      loop.current.ui64 += loop.delta.ui64;
      finished = variable.primitive.ui64 > loop.target.ui64;
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
  uint16_t const index = instruction_->index;

  // Note: the parameters are already on the stack
  Frame frame;
  frame.function = function_;
  frame.bsp      = bsp_;
  frame.pc       = pc_;
  if (frame_sp_ >= FRAME_STACK_SIZE - 1)
  {
    RuntimeError("frame stack overflow");
    return;
  }
  frame_stack_[++frame_sp_] = frame;
  function_                 = &(executable_->functions[index]);
  bsp_                      = sp_ - function_->num_parameters + 1;  // first parameter
  pc_                       = 0;
  int const num_locals      = function_->num_variables - function_->num_parameters;
  sp_ += num_locals;
}

void VM::Handler__VariablePrefixInc()
{
  DoVariablePrefixPostfixOp<PrefixInc>();
}

void VM::Handler__VariablePrefixDec()
{
  DoVariablePrefixPostfixOp<PrefixDec>();
}

void VM::Handler__VariablePostfixInc()
{
  DoVariablePrefixPostfixOp<PostfixInc>();
}

void VM::Handler__VariablePostfixDec()
{
  DoVariablePrefixPostfixOp<PostfixDec>();
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
    top.object->Negate(top.object);
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

void VM::Handler__VariablePrimitiveInplaceAdd()
{
  DoVariableNumericInplaceOp<PrimitiveAdd>();
}

void VM::Handler__VariableObjectInplaceAdd()
{
  DoVariableObjectInplaceOp<ObjectInplaceAdd>();
}

void VM::Handler__VariableObjectInplaceRightAdd()
{
  DoVariableObjectInplaceRightOp<ObjectInplaceRightAdd>();
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

void VM::Handler__VariablePrimitiveInplaceSubtract()
{
  DoVariableNumericInplaceOp<PrimitiveSubtract>();
}

void VM::Handler__VariableObjectInplaceSubtract()
{
  DoVariableObjectInplaceOp<ObjectInplaceSubtract>();
}

void VM::Handler__VariableObjectInplaceRightSubtract()
{
  DoVariableObjectInplaceRightOp<ObjectInplaceRightSubtract>();
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

void VM::Handler__VariablePrimitiveInplaceMultiply()
{
  DoVariableNumericInplaceOp<PrimitiveMultiply>();
}

void VM::Handler__VariableObjectInplaceMultiply()
{
  DoVariableObjectInplaceOp<ObjectInplaceMultiply>();
}

void VM::Handler__VariableObjectInplaceRightMultiply()
{
  DoVariableObjectInplaceRightOp<ObjectInplaceRightMultiply>();
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

void VM::Handler__VariablePrimitiveInplaceDivide()
{
  DoVariableNumericInplaceOp<PrimitiveDivide>();
}

void VM::Handler__VariableObjectInplaceDivide()
{
  DoVariableObjectInplaceOp<ObjectInplaceDivide>();
}

void VM::Handler__VariableObjectInplaceRightDivide()
{
  DoVariableObjectInplaceRightOp<ObjectInplaceRightDivide>();
}

void VM::Handler__PrimitiveModulo()
{
  DoIntegralOp<PrimitiveModulo>();
}

void VM::Handler__VariablePrimitiveInplaceModulo()
{
  DoVariableIntegralInplaceOp<PrimitiveModulo>();
}

}  // namespace vm
}  // namespace fetch
