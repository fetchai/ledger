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

//
// Opcode Handlers
//

void VM::VarDeclare()
{
  Variant &variable = GetVariable(instruction_->index);
  if (instruction_->data.i32 != -1)
  {
    variable.Construct(Ptr<Object>(), instruction_->type_id);
    LiveObjectInfo &info = live_object_stack_[++live_object_sp_];
    info.frame_sp        = frame_sp_;
    info.variable_index  = instruction_->index;
    info.scope_number    = instruction_->data.i32;
  }
  else
  {
    Primitive p;
    p.Zero();
    variable.Construct(p, instruction_->type_id);
  }
}

void VM::VarDeclareAssign()
{
  Variant &variable = GetVariable(instruction_->index);
  variable          = std::move(Pop());
  if (instruction_->data.i32 != -1)
  {
    LiveObjectInfo &info = live_object_stack_[++live_object_sp_];
    info.frame_sp        = frame_sp_;
    info.variable_index  = instruction_->index;
    info.scope_number    = instruction_->data.i32;
  }
}

void VM::PushConstant()
{
  Variant &top = Push();
  top.Construct(instruction_->data, instruction_->type_id);
}

void VM::PushString()
{
  Variant &top = Push();
  top.Construct(strings_[instruction_->index], TypeIds::String);
}

void VM::PushNull()
{
  Variant &top = Push();
  top.Construct(Ptr<Object>(), instruction_->type_id);
}

void VM::PushVariable()
{
  Variant const &variable = GetVariable(instruction_->index);
  Variant &      top      = Push();
  top.Construct(variable);
}

void VM::PushElement()
{
  Variant &container = Pop();
  if (container.object)
  {
    container.object->PushElement(instruction_->type_id);
    container.Reset();
    return;
  }
  RuntimeError("null reference");
}

void VM::PopToVariable()
{
  Variant &variable = GetVariable(instruction_->index);
  variable          = std::move(Pop());
}

void VM::PopToElement()
{
  Variant &container = Pop();
  if (container.object)
  {
    container.object->PopToElement();
    container.Reset();
    return;
  }
  RuntimeError("null reference");
}

void VM::Discard()
{
  Pop().Reset();
}

void VM::Destruct()
{
  Destruct(instruction_->data.i32);
}

void VM::Break()
{
  Destruct(instruction_->data.i32);
  pc_ = instruction_->index;
}

void VM::Continue()
{
  Destruct(instruction_->data.i32);
  pc_ = instruction_->index;
}

void VM::Jump()
{
  pc_ = instruction_->index;
}

void VM::JumpIfFalse()
{
  Variant &v = Pop();
  if (v.primitive.ui8 == 0)
  {
    pc_ = instruction_->index;
  }
  v.Reset();
}

void VM::JumpIfTrue()
{
  Variant &v = Pop();
  if (v.primitive.ui8 != 0)
  {
    pc_ = instruction_->index;
  }
  v.Reset();
}

// NOTE: Opcodes::Return and Opcodes::ReturnValue both route through here
void VM::Return()
{
  Destruct(instruction_->data.i32);
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

void VM::ToInt8()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.i8);
}

void VM::ToByte()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.ui8);
}

void VM::ToInt16()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.i16);
}

void VM::ToUInt16()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.ui16);
}

void VM::ToInt32()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.i32);
}

void VM::ToUInt32()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.ui32);
}

void VM::ToInt64()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.i64);
}

void VM::ToUInt64()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.ui64);
}

void VM::ToFloat32()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.f32);
}

void VM::ToFloat64()
{
  Variant &top = Top();
  Cast(top, instruction_->type_id, top.primitive.f64);
}

void VM::ForRangeInit()
{
  ForRangeLoop loop;
  loop.variable_index = instruction_->index;
  Variant &variable   = GetVariable(loop.variable_index);
  variable.type_id    = instruction_->type_id;
  if (instruction_->data.i32 == 2)
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

void VM::ForRangeIterate()
{
  ForRangeLoop &loop     = range_loop_stack_[range_loop_sp_];
  Variant &     variable = GetVariable(loop.variable_index);
  bool          finished = true;
  if (instruction_->data.i32 == 2)
  {
    switch (variable.type_id)
    {
    case TypeIds::Int8:
    {
      variable.primitive.i8 = loop.current.i8++;
      finished              = variable.primitive.i8 > loop.target.i8;
      break;
    }
    case TypeIds::Byte:
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
    case TypeIds::Byte:
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

void VM::ForRangeTerminate()
{
  --range_loop_sp_;
}

void VM::InvokeUserFunction()
{
  Index const index = instruction_->index;

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
  function_                 = &(script_->functions[index]);
  bsp_                      = sp_ - function_->num_parameters + 1;  // first parameter
  pc_                       = 0;
  int const num_locals      = function_->num_variables - function_->num_parameters;
  sp_ += num_locals;
}

void VM::Equal()
{
  DoRelationalOp<EqualOp>();
}

void VM::ObjectEqual()
{
  Variant &rhsv = Pop();
  Variant &lhsv = Top();
  lhsv.Assign(IsEqual(lhsv.object, rhsv.object), TypeIds::Bool);
  rhsv.Reset();
}

void VM::NotEqual()
{
  DoRelationalOp<NotEqualOp>();
}

void VM::ObjectNotEqual()
{
  Variant &rhsv = Pop();
  Variant &lhsv = Top();
  lhsv.Assign(IsNotEqual(lhsv.object, rhsv.object), TypeIds::Bool);
  rhsv.Reset();
}

void VM::LessThan()
{
  DoRelationalOp<LessThanOp>();
}

void VM::ObjectLessThan()
{
  DoObjectRelationalOp<ObjectLessThanOp>();
}

void VM::LessThanOrEqual()
{
  DoRelationalOp<LessThanOrEqualOp>();
}

void VM::ObjectLessThanOrEqual()
{
  DoObjectRelationalOp<ObjectLessThanOrEqualOp>();
}

void VM::GreaterThan()
{
  DoRelationalOp<GreaterThanOp>();
}

void VM::ObjectGreaterThan()
{
  DoObjectRelationalOp<ObjectGreaterThanOp>();
}

void VM::GreaterThanOrEqual()
{
  DoRelationalOp<GreaterThanOrEqualOp>();
}

void VM::ObjectGreaterThanOrEqual()
{
  DoObjectRelationalOp<ObjectGreaterThanOrEqualOp>();
}

void VM::And()
{
  Variant &rhsv = Pop();
  Variant &lhsv = Top();
  lhsv.primitive.ui8 &= rhsv.primitive.ui8;
  rhsv.Reset();
}

void VM::Or()
{
  Variant &rhsv = Pop();
  Variant &lhsv = Top();
  lhsv.primitive.ui8 |= rhsv.primitive.ui8;
  rhsv.Reset();
}

void VM::Not()
{
  Variant &top      = Top();
  top.primitive.ui8 = top.primitive.ui8 == 0 ? 1 : 0;
}

void VM::VariablePrefixInc()
{
  DoVariableIncDecOp<PrefixIncOp>();
}

void VM::VariablePrefixDec()
{
  DoVariableIncDecOp<PrefixDecOp>();
}

void VM::VariablePostfixInc()
{
  DoVariableIncDecOp<PostfixIncOp>();
}

void VM::VariablePostfixDec()
{
  DoVariableIncDecOp<PostfixDecOp>();
}

void VM::ElementPrefixInc()
{
  DoElementIncDecOp<PrefixIncOp>();
}

void VM::ElementPrefixDec()
{
  DoElementIncDecOp<PrefixDecOp>();
}

void VM::ElementPostfixInc()
{
  DoElementIncDecOp<PostfixIncOp>();
}

void VM::ElementPostfixDec()
{
  DoElementIncDecOp<PostfixDecOp>();
}

void VM::Modulo()
{
  DoIntegerOp<ModuloOp>();
}

void VM::VariableModuloAssign()
{
  DoVariableIntegerAssignOp<ModuloOp>();
}

void VM::ElementModuloAssign()
{
  DoElementIntegerAssignOp<ModuloOp>();
}

void VM::UnaryMinus()
{
  Variant &top = Top();
  ExecuteNumberOp<UnaryMinusOp>(instruction_->type_id, top, top);
}

void VM::ObjectUnaryMinus()
{
  Variant &top = Top();
  if (top.object)
  {
    top.object->UnaryMinus(top.object);
    return;
  }
  RuntimeError("null reference");
}

void VM::Add()
{
  DoNumberOp<AddOp>();
}

void VM::LeftAdd()
{
  DoLeftOp<LeftAddOp>();
}

void VM::RightAdd()
{
  DoRightOp<RightAddOp>();
}

void VM::ObjectAdd()
{
  DoObjectOp<ObjectAddOp>();
}

void VM::VariableAddAssign()
{
  DoVariableNumberAssignOp<AddOp>();
}

void VM::VariableRightAddAssign()
{
  DoVariableRightAssignOp<RightAddAssignOp>();
}

void VM::VariableObjectAddAssign()
{
  DoVariableObjectAssignOp<ObjectAddAssignOp>();
}

void VM::ElementAddAssign()
{
  DoElementNumberAssignOp<AddOp>();
}

void VM::ElementRightAddAssign()
{
  DoElementRightAssignOp<RightAddAssignOp>();
}

void VM::ElementObjectAddAssign()
{
  DoElementObjectAssignOp<ObjectAddAssignOp>();
}

void VM::Subtract()
{
  DoNumberOp<SubtractOp>();
}

void VM::LeftSubtract()
{
  DoLeftOp<LeftSubtractOp>();
}

void VM::RightSubtract()
{
  DoRightOp<RightSubtractOp>();
}

void VM::ObjectSubtract()
{
  DoObjectOp<ObjectSubtractOp>();
}

void VM::VariableSubtractAssign()
{
  DoVariableNumberAssignOp<SubtractOp>();
}

void VM::VariableRightSubtractAssign()
{
  DoVariableRightAssignOp<RightSubtractAssignOp>();
}

void VM::VariableObjectSubtractAssign()
{
  DoVariableObjectAssignOp<ObjectSubtractAssignOp>();
}

void VM::ElementSubtractAssign()
{
  DoElementNumberAssignOp<SubtractOp>();
}

void VM::ElementRightSubtractAssign()
{
  DoElementRightAssignOp<RightSubtractAssignOp>();
}

void VM::ElementObjectSubtractAssign()
{
  DoElementObjectAssignOp<ObjectSubtractAssignOp>();
}

void VM::Multiply()
{
  DoNumberOp<MultiplyOp>();
}

void VM::LeftMultiply()
{
  DoLeftOp<LeftMultiplyOp>();
}

void VM::RightMultiply()
{
  DoRightOp<RightMultiplyOp>();
}

void VM::ObjectMultiply()
{
  DoObjectOp<ObjectMultiplyOp>();
}

void VM::VariableMultiplyAssign()
{
  DoVariableNumberAssignOp<MultiplyOp>();
}

void VM::VariableRightMultiplyAssign()
{
  DoVariableRightAssignOp<RightMultiplyAssignOp>();
}

void VM::VariableObjectMultiplyAssign()
{
  DoVariableObjectAssignOp<ObjectMultiplyAssignOp>();
}

void VM::ElementMultiplyAssign()
{
  DoElementNumberAssignOp<MultiplyOp>();
}

void VM::ElementRightMultiplyAssign()
{
  DoElementRightAssignOp<RightMultiplyAssignOp>();
}

void VM::ElementObjectMultiplyAssign()
{
  DoElementObjectAssignOp<ObjectMultiplyAssignOp>();
}

void VM::Divide()
{
  DoNumberOp<DivideOp>();
}

void VM::LeftDivide()
{
  DoLeftOp<LeftDivideOp>();
}

void VM::RightDivide()
{
  DoRightOp<RightDivideOp>();
}

void VM::ObjectDivide()
{
  DoObjectOp<ObjectDivideOp>();
}

void VM::VariableDivideAssign()
{
  DoVariableNumberAssignOp<DivideOp>();
}

void VM::VariableRightDivideAssign()
{
  DoVariableRightAssignOp<RightDivideAssignOp>();
}

void VM::VariableObjectDivideAssign()
{
  DoVariableObjectAssignOp<ObjectDivideAssignOp>();
}

void VM::ElementDivideAssign()
{
  DoElementNumberAssignOp<DivideOp>();
}

void VM::ElementRightDivideAssign()
{
  DoElementRightAssignOp<RightDivideAssignOp>();
}

void VM::ElementObjectDivideAssign()
{
  DoElementObjectAssignOp<ObjectDivideAssignOp>();
}

}  // namespace vm
}  // namespace fetch
