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

#include "vm/vm.hpp"
#include "vm/module.hpp"
#include <sstream>

namespace fetch {
namespace vm {

void Object::Release()
{
  if (--count == 0)
  {
    vm->ReleaseObject(this, type_id);
  }
}

void VM::RuntimeError(const std::string &message)
{
  std::stringstream stream;
  stream << "runtime error: " << message;
  error_      = stream.str();
  error_line_ = std::size_t(this->instruction_->line);
  stop_       = true;
}

void VM::AcquireMatrix(const size_t rows, const size_t columns, MatrixFloat32 *&m)
{
  m = new MatrixFloat32(TypeId::Matrix_Float32, this, rows, columns);
}

void VM::AcquireMatrix(const size_t rows, const size_t columns, MatrixFloat64 *&m)
{
  m = new MatrixFloat64(TypeId::Matrix_Float64, this, rows, columns);
}

void VM::ForRangeInit()
{
  ForRangeLoop loop;
  loop.variable_index = instruction_->index;
  Value &variable     = GetVariable(loop.variable_index);
  variable.type_id    = instruction_->type_id;
  if (instruction_->variant.i32 == 2)
  {
    Value &targetv = stack_[sp_--];
    Value &startv  = stack_[sp_--];
    loop.current   = startv.variant;
    loop.target    = targetv.variant;
    targetv.PrimitiveReset();
    startv.PrimitiveReset();
  }
  else
  {
    Value &deltav  = stack_[sp_--];
    Value &targetv = stack_[sp_--];
    Value &startv  = stack_[sp_--];
    loop.current   = startv.variant;
    loop.target    = targetv.variant;
    loop.delta     = deltav.variant;
    deltav.PrimitiveReset();
    targetv.PrimitiveReset();
    startv.PrimitiveReset();
  }
  range_loop_stack_[++range_loop_sp_] = loop;
}

void VM::ForRangeIterate()
{
  ForRangeLoop &loop     = range_loop_stack_[range_loop_sp_];
  Value &       variable = GetVariable(loop.variable_index);
  bool          finished = true;
  if (instruction_->variant.i32 == 2)
  {
    switch (variable.type_id)
    {
    case TypeId::Int8:
    {
      variable.variant.i8 = loop.current.i8++;
      finished            = variable.variant.i8 > loop.target.i8;
      break;
    }
    case TypeId::Byte:
    {
      variable.variant.ui8 = loop.current.ui8++;
      finished             = variable.variant.ui8 > loop.target.ui8;
      break;
    }
    case TypeId::Int16:
    {
      variable.variant.i16 = loop.current.i16++;
      finished             = variable.variant.i16 > loop.target.i16;
      break;
    }
    case TypeId::UInt16:
    {
      variable.variant.ui16 = loop.current.ui16++;
      finished              = variable.variant.ui16 > loop.target.ui16;
      break;
    }
    case TypeId::Int32:
    {
      variable.variant.i32 = loop.current.i32++;
      finished             = variable.variant.i32 > loop.target.i32;
      break;
    }
    case TypeId::UInt32:
    {
      variable.variant.ui32 = loop.current.ui32++;
      finished              = variable.variant.ui32 > loop.target.ui32;
      break;
    }
    case TypeId::Int64:
    {
      variable.variant.i64 = loop.current.i64++;
      finished             = variable.variant.i64 > loop.target.i64;
      break;
    }
    case TypeId::UInt64:
    {
      variable.variant.ui64 = loop.current.ui64++;
      finished              = variable.variant.ui64 > loop.target.ui64;
      break;
    }
    default:
    {
      break;
    }
    }
  }
  else
  {
    switch (variable.type_id)
    {
    case TypeId::Int8:
    {
      variable.variant.i8 = loop.current.i8;
      loop.current.i8     = int8_t(loop.current.i8 + loop.delta.i8);
      finished            = (loop.delta.i8 >= 0) ? variable.variant.i8 > loop.target.i8
                                      : variable.variant.i8 < loop.target.i8;
      break;
    }
    case TypeId::Byte:
    {
      variable.variant.ui8 = loop.current.ui8;
      loop.current.ui8     = uint8_t(loop.current.ui8 + loop.delta.ui8);
      finished             = variable.variant.ui8 > loop.target.ui8;
      break;
    }
    case TypeId::Int16:
    {
      variable.variant.i16 = loop.current.i16;
      loop.current.i16     = int16_t(loop.current.i16 + loop.delta.i16);
      finished             = (loop.delta.i16 >= 0) ? variable.variant.i16 > loop.target.i16
                                       : variable.variant.i16 < loop.target.i16;
      break;
    }
    case TypeId::UInt16:
    {
      variable.variant.ui16 = loop.current.ui16;
      loop.current.ui16     = uint16_t(loop.current.ui16 + loop.delta.ui16);
      finished              = variable.variant.ui16 > loop.target.ui16;
    }
    case TypeId::Int32:
    {
      variable.variant.i32 = loop.current.i32;
      loop.current.i32 += loop.delta.i32;
      finished = (loop.delta.i32 >= 0) ? variable.variant.i32 > loop.target.i32
                                       : variable.variant.i32 < loop.target.i32;
      break;
    }
    case TypeId::UInt32:
    {
      variable.variant.ui32 = loop.current.ui32;
      loop.current.ui32 += loop.delta.ui32;
      finished = variable.variant.ui32 > loop.target.ui32;
      break;
    }
    case TypeId::Int64:
    {
      variable.variant.i64 = loop.current.i64;
      loop.current.i64 += loop.delta.i64;
      finished = (loop.delta.i64 >= 0) ? variable.variant.i64 > loop.target.i64
                                       : variable.variant.i64 < loop.target.i64;
      break;
    }
    case TypeId::UInt64:
    {
      variable.variant.ui64 = loop.current.ui64;
      loop.current.ui64 += loop.delta.ui64;
      finished = variable.variant.ui64 > loop.target.ui64;
      break;
    }
    default:
    {
      break;
    }
    }
  }
  if (finished)
    pc_ = instruction_->index;
}

void VM::CreateMatrix()
{
  Value &w = stack_[sp_--];
  Value &h = stack_[sp_];
  if ((w.variant.i32 < 0) || (h.variant.i32 < 0))
  {
    RuntimeError("negative size");
    return;
  }
  const uint64_t columns = uint64_t(w.variant.i32);
  const uint64_t rows    = uint64_t(h.variant.i32);
  w.Reset();
  const TypeId type_id = instruction_->type_id;
  if (type_id == TypeId::Matrix_Float32)
  {
    MatrixFloat32 *m;
    AcquireMatrix(rows, columns, m);
    h.SetObject(m, type_id);
  }
  else
  {
    MatrixFloat64 *m;
    AcquireMatrix(rows, columns, m);
    h.SetObject(m, type_id);
  }
}

void VM::CreateArray()
{
  Value &top = stack_[sp_];
  if (top.variant.i32 < 0)
  {
    RuntimeError("negative size");
    return;
  }
  const uint64_t size    = uint64_t(top.variant.i32);
  const TypeId   type_id = instruction_->type_id;
  Object *       object  = nullptr;
  switch (type_id)
  {
  case TypeId::Array_Int8:
  {
    object = new Array<int8_t>(type_id, this, size);
    break;
  }
  case TypeId::Array_Bool:
  case TypeId::Array_Byte:
  {
    object = new Array<uint8_t>(type_id, this, size);
    break;
  }
  case TypeId::Array_Int16:
  {
    object = new Array<int16_t>(type_id, this, size);
    break;
  }
  case TypeId::Array_UInt16:
  {
    object = new Array<uint16_t>(type_id, this, size);
    break;
  }
  case TypeId::Array_Int32:
  {
    object = new Array<int32_t>(type_id, this, size);
    break;
  }
  case TypeId::Array_UInt32:
  {
    object = new Array<uint32_t>(type_id, this, size);
    break;
  }
  case TypeId::Array_Int64:
  {
    object = new Array<int64_t>(type_id, this, size);
    break;
  }
  case TypeId::Array_UInt64:
  {
    object = new Array<uint64_t>(type_id, this, size);
    break;
  }
  case TypeId::Array_Float32:
  {
    object = new Array<float>(type_id, this, size);
    break;
  }
  case TypeId::Array_Float64:
  {
    object = new Array<double>(type_id, this, size);
    break;
  }
  case TypeId::Array_Matrix_Float32:
  case TypeId::Array_Matrix_Float64:
  case TypeId::Array_String:
  case TypeId::Array:
  {
    object = new Array<Object *>(type_id, this, size);
    break;
  }
  default:
    break;
  }
  top.SetObject(object, type_id);
}

bool VM::Execute(const Script &script, const std::string &name)
{
  const Script::Function *f = script.FindFunction(name);
  if (f == nullptr)
    return false;
  script_                       = &script;
  function_                     = f;
  const std::size_t num_strings = script_->strings.size();
  pool_                         = std::vector<String>(num_strings);
  strings_                      = std::vector<String *>(num_strings);
  for (std::size_t i = 0; i < num_strings; ++i)
  {
    const std::string &str = script_->strings[i];
    pool_[i]               = String(this, str, true);
    strings_[i]            = &pool_[i];
  }
  frame_sp_       = -1;
  bsp_            = 0;
  sp_             = function_->num_variables - 1;
  range_loop_sp_  = -1;
  live_object_sp_ = -1;
  pc_             = 0;
  instruction_    = nullptr;
  stop_           = false;
  error_.clear();

  do
  {
    instruction_ = &function_->instructions[std::size_t(pc_)];
    ++pc_;

    switch (instruction_->opcode)
    {
    case Opcode::Jump:
    {
      pc_ = instruction_->index;
      break;
    }
    case Opcode::JumpIfFalse:
    {
      Value &value = stack_[sp_--];
      if (value.variant.ui8 == 0)
        pc_ = instruction_->index;
      value.PrimitiveReset();
      break;
    }
    case Opcode::PushString:
    {
      Value &value         = stack_[++sp_];
      value.type_id        = TypeId::String;
      value.variant.object = strings_[instruction_->index];
      value.variant.object->AddRef();
      break;
    }
    case Opcode::PushConstant:
    {
      Value &value  = stack_[++sp_];
      value.type_id = instruction_->type_id;
      value.variant = instruction_->variant;
      break;
    }
    case Opcode::PushVariable:
    {
      const Value &variable = GetVariable(instruction_->index);
      stack_[++sp_].Copy(variable);
      break;
    }
    case Opcode::VarDeclare:
    {
      Value &variable  = GetVariable(instruction_->index);
      variable.type_id = instruction_->type_id;
      variable.variant.Zero();
      if (instruction_->variant.i32 != -1)
      {
        LiveObjectInfo &info = live_object_stack_[++live_object_sp_];
        info.frame_sp        = frame_sp_;
        info.variable_index  = instruction_->index;
        info.scope_number    = instruction_->variant.i32;
      }
      break;
    }
    case Opcode::VarDeclareAssign:
    {
      Value &variable = GetVariable(instruction_->index);
      variable        = std::move(stack_[sp_--]);
      if (instruction_->variant.i32 != -1)
      {
        LiveObjectInfo &info = live_object_stack_[++live_object_sp_];
        info.frame_sp        = frame_sp_;
        info.variable_index  = instruction_->index;
        info.scope_number    = instruction_->variant.i32;
      }
      break;
    }
    case Opcode::Assign:
    {
      Value &variable = GetVariable(instruction_->index);
      variable        = std::move(stack_[sp_--]);
      break;
    }
    case Opcode::Discard:
    {
      stack_[sp_--].Reset();
      break;
    }
    case Opcode::Destruct:
    {
      Destruct(instruction_->variant.i32);
      break;
    }
    case Opcode::InvokeUserFunction:
    {
      InvokeUserFunction(instruction_->index);
      break;
    }
    case Opcode::ForRangeInit:
    {
      ForRangeInit();
      break;
    }
    case Opcode::ForRangeIterate:
    {
      ForRangeIterate();
      break;
    }
    case Opcode::ForRangeTerminate:
    {
      --range_loop_sp_;
      break;
    }
    case Opcode::Break:
    case Opcode::Continue:
    {
      Destruct(instruction_->variant.i32);
      pc_ = instruction_->index;
      break;
    }
    case Opcode::Return:
    case Opcode::ReturnValue:
    {
      Destruct(instruction_->variant.i32);
      if (instruction_->opcode == Opcode::ReturnValue)
      {
        for (int i = bsp_ + 1; i < bsp_ + function_->num_parameters; ++i)
          stack_[i].Reset();
        if (sp_ != bsp_)
          stack_[bsp_] = std::move(stack_[sp_]);
        sp_ = bsp_;
      }
      else
      {
        for (int i = bsp_; i < bsp_ + function_->num_parameters; ++i)
          stack_[i].Reset();
        sp_ = bsp_ - 1;
      }
      if (frame_sp_ != -1)
      {
        const Frame &frame = frame_stack_[frame_sp_];
        function_          = frame.function;
        bsp_               = frame.bsp;
        pc_                = frame.pc;
        --frame_sp_;
      }
      else
      {
        // We've finished executing
        stop_ = true;
      }
      break;
    }
    case Opcode::ToInt8:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::Int8, value.variant.i8);
      break;
    }
    case Opcode::ToByte:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::Byte, value.variant.ui8);
      break;
    }
    case Opcode::ToInt16:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::Int16, value.variant.i16);
      break;
    }
    case Opcode::ToUInt16:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::UInt16, value.variant.ui16);
      break;
    }
    case Opcode::ToInt32:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::Int32, value.variant.i32);
      break;
    }
    case Opcode::ToUInt32:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::UInt32, value.variant.ui32);
      break;
    }
    case Opcode::ToInt64:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::Int64, value.variant.i64);
      break;
    }
    case Opcode::ToUInt64:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::UInt64, value.variant.ui64);
      break;
    }
    case Opcode::ToFloat32:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::Float32, value.variant.f32);
      break;
    }
    case Opcode::ToFloat64:
    {
      Value &value = stack_[sp_];
      HandleCast(value, TypeId::Float64, value.variant.f64);
      break;
    }
    case Opcode::EqualOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleEqualityOp<EqualOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::NotEqualOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleEqualityOp<NotEqualOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::LessThanOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleRelationalOp<LessThanOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::LessThanOrEqualOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleRelationalOp<LessThanOrEqualOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::GreaterThanOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleRelationalOp<GreaterThanOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::GreaterThanOrEqualOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleRelationalOp<GreaterThanOrEqualOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::AndOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      lhsv.variant.ui8 &= rhsv.variant.ui8;
      rhsv.PrimitiveReset();
      break;
    }
    case Opcode::OrOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      lhsv.variant.ui8 |= rhsv.variant.ui8;
      rhsv.PrimitiveReset();
      break;
    }
    case Opcode::NotOp:
    {
      Value &value      = stack_[sp_];
      value.variant.ui8 = value.variant.ui8 == 0 ? 1 : 0;
      break;
    }
    case Opcode::AddOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleArithmeticOp<AddOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::SubtractOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleArithmeticOp<SubtractOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::MultiplyOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleArithmeticOp<MultiplyOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::DivideOp:
    {
      Value &rhsv = stack_[sp_--];
      Value &lhsv = stack_[sp_];
      HandleArithmeticOp<DivideOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::UnaryMinusOp:
    {
      Value &value = stack_[sp_];
      HandleArithmeticOp<UnaryMinusOp>(instruction_->type_id, value, value);
      break;
    }
    case Opcode::AddAssignOp:
    {
      Value &lhsv = GetVariable(instruction_->index);
      Value &rhsv = stack_[sp_--];
      HandleArithmeticAssignmentOp<AddAssignOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::SubtractAssignOp:
    {
      Value &lhsv = GetVariable(instruction_->index);
      Value &rhsv = stack_[sp_--];
      HandleArithmeticAssignmentOp<SubtractAssignOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::MultiplyAssignOp:
    {
      Value &lhsv = GetVariable(instruction_->index);
      Value &rhsv = stack_[sp_--];
      HandleArithmeticAssignmentOp<MultiplyAssignOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::DivideAssignOp:
    {
      Value &lhsv = GetVariable(instruction_->index);
      Value &rhsv = stack_[sp_--];
      HandleArithmeticAssignmentOp<DivideAssignOp>(instruction_->type_id, lhsv, rhsv);
      rhsv.Reset();
      break;
    }
    case Opcode::PrefixIncOp:
    {
      Value &lhsv = stack_[++sp_];
      Value &rhsv = GetVariable(instruction_->index);
      HandleArithmeticAssignmentOp<PrefixIncOp>(instruction_->type_id, lhsv, rhsv);
      lhsv.type_id = instruction_->type_id;
      break;
    }
    case Opcode::PrefixDecOp:
    {
      Value &lhsv = stack_[++sp_];
      Value &rhsv = GetVariable(instruction_->index);
      HandleArithmeticAssignmentOp<PrefixDecOp>(instruction_->type_id, lhsv, rhsv);
      lhsv.type_id = instruction_->type_id;
      break;
    }
    case Opcode::PostfixIncOp:
    {
      Value &lhsv = stack_[++sp_];
      Value &rhsv = GetVariable(instruction_->index);
      HandleArithmeticAssignmentOp<PostfixIncOp>(instruction_->type_id, lhsv, rhsv);
      lhsv.type_id = instruction_->type_id;
      break;
    }
    case Opcode::PostfixDecOp:
    {
      Value &lhsv = stack_[++sp_];
      Value &rhsv = GetVariable(instruction_->index);
      HandleArithmeticAssignmentOp<PostfixDecOp>(instruction_->type_id, lhsv, rhsv);
      lhsv.type_id = instruction_->type_id;
      break;
    }
    case Opcode::IndexedAssign:
    {
      HandleIndexedAssignment(instruction_->type_id);
      break;
    }
    case Opcode::IndexOp:
    {
      HandleIndexOp(instruction_->type_id);
      break;
    }
    case Opcode::IndexedAddAssignOp:
    {
      HandleIndexedArithmeticAssignmentOp<AddAssignOp>(instruction_->type_id);
      break;
    }
    case Opcode::IndexedSubtractAssignOp:
    {
      HandleIndexedArithmeticAssignmentOp<SubtractAssignOp>(instruction_->type_id);
      break;
    }
    case Opcode::IndexedMultiplyAssignOp:
    {
      HandleIndexedArithmeticAssignmentOp<MultiplyAssignOp>(instruction_->type_id);
      break;
    }
    case Opcode::IndexedDivideAssignOp:
    {
      HandleIndexedArithmeticAssignmentOp<DivideAssignOp>(instruction_->type_id);
      break;
    }
    case Opcode::IndexedPrefixIncOp:
    {
      HandleIndexedPrefixPostfixOp<PrefixIncOp>(instruction_->type_id);
      break;
    }
    case Opcode::IndexedPrefixDecOp:
    {
      HandleIndexedPrefixPostfixOp<PrefixDecOp>(instruction_->type_id);
      break;
    }
    case Opcode::IndexedPostfixIncOp:
    {
      HandleIndexedPrefixPostfixOp<PostfixIncOp>(instruction_->type_id);
      break;
    }
    case Opcode::IndexedPostfixDecOp:
    {
      HandleIndexedPrefixPostfixOp<PostfixDecOp>(instruction_->type_id);
      break;
    }
    case Opcode::CreateMatrix:
    {
      CreateMatrix();
      break;
    }
    case Opcode::CreateArray:
    {
      CreateArray();
      break;
    }

    default:
    {
      // If the operation does not match any of the builtin operations
      // try to match them against operations defined in the module
      if (module_ != nullptr)
      {
        if (!module_->ExecuteUserOpcode(this, instruction_->opcode))
        {
          RuntimeError("unknown opcode");
        }
      }
      else
      {
        RuntimeError("unknown opcode");
      }
      break;
    }
    }
  } while (stop_ == false);

  if (error_.empty())
  {
    return true;
  }

  // We've got a runtime error
  // Reset all variables on the current stack
  for (int i = 0; i <= sp_; ++i)
    stack_[i].Reset();
  return false;
}

}  // namespace vm
}  // namespace fetch
