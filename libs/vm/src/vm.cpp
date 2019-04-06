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
#include "vm/module.hpp"
#include <sstream>

namespace fetch {
namespace vm {

VM::VM(Module *module)
{
  std::vector<OpcodeHandlerInfo> array = {
      {Opcodes::VarDeclare, [](VM *vm) { vm->VarDeclare(); }},
      {Opcodes::VarDeclareAssign, [](VM *vm) { vm->VarDeclareAssign(); }},
      {Opcodes::PushConstant, [](VM *vm) { vm->PushConstant(); }},
      {Opcodes::PushString, [](VM *vm) { vm->PushString(); }},
      {Opcodes::PushNull, [](VM *vm) { vm->PushNull(); }},
      {Opcodes::PushVariable, [](VM *vm) { vm->PushVariable(); }},
      {Opcodes::PushElement, [](VM *vm) { vm->PushElement(); }},
      {Opcodes::PopToVariable, [](VM *vm) { vm->PopToVariable(); }},
      {Opcodes::PopToElement, [](VM *vm) { vm->PopToElement(); }},
      {Opcodes::Discard, [](VM *vm) { vm->Discard(); }},
      {Opcodes::Destruct, [](VM *vm) { vm->Destruct(); }},
      {Opcodes::Break, [](VM *vm) { vm->Break(); }},
      {Opcodes::Continue, [](VM *vm) { vm->Continue(); }},
      {Opcodes::Jump, [](VM *vm) { vm->Jump(); }},
      {Opcodes::JumpIfFalse, [](VM *vm) { vm->JumpIfFalse(); }},
      {Opcodes::JumpIfTrue, [](VM *vm) { vm->JumpIfTrue(); }},
      {Opcodes::Return, [](VM *vm) { vm->Return(); }},
      {Opcodes::ReturnValue, [](VM *vm) { vm->Return(); }},
      {Opcodes::ToInt8, [](VM *vm) { vm->ToInt8(); }},
      {Opcodes::ToByte, [](VM *vm) { vm->ToByte(); }},
      {Opcodes::ToInt16, [](VM *vm) { vm->ToInt16(); }},
      {Opcodes::ToUInt16, [](VM *vm) { vm->ToUInt16(); }},
      {Opcodes::ToInt32, [](VM *vm) { vm->ToInt32(); }},
      {Opcodes::ToUInt32, [](VM *vm) { vm->ToUInt32(); }},
      {Opcodes::ToInt64, [](VM *vm) { vm->ToInt64(); }},
      {Opcodes::ToUInt64, [](VM *vm) { vm->ToUInt64(); }},
      {Opcodes::ToFloat32, [](VM *vm) { vm->ToFloat32(); }},
      {Opcodes::ToFloat64, [](VM *vm) { vm->ToFloat64(); }},
      {Opcodes::ForRangeInit, [](VM *vm) { vm->ForRangeInit(); }},
      {Opcodes::ForRangeIterate, [](VM *vm) { vm->ForRangeIterate(); }},
      {Opcodes::ForRangeTerminate, [](VM *vm) { vm->ForRangeTerminate(); }},
      {Opcodes::InvokeUserFunction, [](VM *vm) { vm->InvokeUserFunction(); }},
      {Opcodes::Equal, [](VM *vm) { vm->Equal(); }},
      {Opcodes::ObjectEqual, [](VM *vm) { vm->ObjectEqual(); }},
      {Opcodes::NotEqual, [](VM *vm) { vm->NotEqual(); }},
      {Opcodes::ObjectNotEqual, [](VM *vm) { vm->ObjectNotEqual(); }},
      {Opcodes::LessThan, [](VM *vm) { vm->LessThan(); }},
      {Opcodes::ObjectLessThan, [](VM *vm) { vm->ObjectLessThan(); }},
      {Opcodes::LessThanOrEqual, [](VM *vm) { vm->LessThanOrEqual(); }},
      {Opcodes::ObjectLessThanOrEqual, [](VM *vm) { vm->ObjectLessThanOrEqual(); }},
      {Opcodes::GreaterThan, [](VM *vm) { vm->GreaterThan(); }},
      {Opcodes::ObjectGreaterThan, [](VM *vm) { vm->ObjectGreaterThan(); }},
      {Opcodes::GreaterThanOrEqual, [](VM *vm) { vm->GreaterThanOrEqual(); }},
      {Opcodes::ObjectGreaterThanOrEqual, [](VM *vm) { vm->ObjectGreaterThanOrEqual(); }},
      {Opcodes::And, [](VM *vm) { vm->And(); }},
      {Opcodes::Or, [](VM *vm) { vm->Or(); }},
      {Opcodes::Not, [](VM *vm) { vm->Not(); }},
      {Opcodes::VariablePrefixInc, [](VM *vm) { vm->VariablePrefixInc(); }},
      {Opcodes::VariablePrefixDec, [](VM *vm) { vm->VariablePrefixDec(); }},
      {Opcodes::VariablePostfixInc, [](VM *vm) { vm->VariablePostfixInc(); }},
      {Opcodes::VariablePostfixDec, [](VM *vm) { vm->VariablePostfixDec(); }},
      {Opcodes::ElementPrefixInc, [](VM *vm) { vm->ElementPrefixInc(); }},
      {Opcodes::ElementPrefixDec, [](VM *vm) { vm->ElementPrefixDec(); }},
      {Opcodes::ElementPostfixInc, [](VM *vm) { vm->ElementPostfixInc(); }},
      {Opcodes::ElementPostfixDec, [](VM *vm) { vm->ElementPostfixDec(); }},
      {Opcodes::Modulo, [](VM *vm) { vm->Modulo(); }},
      {Opcodes::VariableModuloAssign, [](VM *vm) { vm->VariableModuloAssign(); }},
      {Opcodes::ElementModuloAssign, [](VM *vm) { vm->ElementModuloAssign(); }},
      {Opcodes::UnaryMinus, [](VM *vm) { vm->UnaryMinus(); }},
      {Opcodes::ObjectUnaryMinus, [](VM *vm) { vm->ObjectUnaryMinus(); }},
      {Opcodes::Add, [](VM *vm) { vm->Add(); }},
      {Opcodes::LeftAdd, [](VM *vm) { vm->LeftAdd(); }},
      {Opcodes::RightAdd, [](VM *vm) { vm->RightAdd(); }},
      {Opcodes::ObjectAdd, [](VM *vm) { vm->ObjectAdd(); }},
      {Opcodes::VariableAddAssign, [](VM *vm) { vm->VariableAddAssign(); }},
      {Opcodes::VariableRightAddAssign, [](VM *vm) { vm->VariableRightAddAssign(); }},
      {Opcodes::VariableObjectAddAssign, [](VM *vm) { vm->VariableObjectAddAssign(); }},
      {Opcodes::ElementAddAssign, [](VM *vm) { vm->ElementAddAssign(); }},
      {Opcodes::ElementRightAddAssign, [](VM *vm) { vm->ElementRightAddAssign(); }},
      {Opcodes::ElementObjectAddAssign, [](VM *vm) { vm->ElementObjectAddAssign(); }},
      {Opcodes::Subtract, [](VM *vm) { vm->Subtract(); }},
      {Opcodes::LeftSubtract, [](VM *vm) { vm->LeftSubtract(); }},
      {Opcodes::RightSubtract, [](VM *vm) { vm->RightSubtract(); }},
      {Opcodes::ObjectSubtract, [](VM *vm) { vm->ObjectSubtract(); }},
      {Opcodes::VariableSubtractAssign, [](VM *vm) { vm->VariableSubtractAssign(); }},
      {Opcodes::VariableRightSubtractAssign, [](VM *vm) { vm->VariableRightSubtractAssign(); }},
      {Opcodes::VariableObjectSubtractAssign, [](VM *vm) { vm->VariableObjectSubtractAssign(); }},
      {Opcodes::ElementSubtractAssign, [](VM *vm) { vm->ElementSubtractAssign(); }},
      {Opcodes::ElementRightSubtractAssign, [](VM *vm) { vm->ElementRightSubtractAssign(); }},
      {Opcodes::ElementObjectSubtractAssign, [](VM *vm) { vm->ElementObjectSubtractAssign(); }},
      {Opcodes::Multiply, [](VM *vm) { vm->Multiply(); }},
      {Opcodes::LeftMultiply, [](VM *vm) { vm->LeftMultiply(); }},
      {Opcodes::RightMultiply, [](VM *vm) { vm->RightMultiply(); }},
      {Opcodes::ObjectMultiply, [](VM *vm) { vm->ObjectMultiply(); }},
      {Opcodes::VariableMultiplyAssign, [](VM *vm) { vm->VariableMultiplyAssign(); }},
      {Opcodes::VariableRightMultiplyAssign, [](VM *vm) { vm->VariableRightMultiplyAssign(); }},
      {Opcodes::VariableObjectMultiplyAssign, [](VM *vm) { vm->VariableObjectMultiplyAssign(); }},
      {Opcodes::ElementMultiplyAssign, [](VM *vm) { vm->ElementMultiplyAssign(); }},
      {Opcodes::ElementRightMultiplyAssign, [](VM *vm) { vm->ElementRightMultiplyAssign(); }},
      {Opcodes::ElementObjectMultiplyAssign, [](VM *vm) { vm->ElementObjectMultiplyAssign(); }},
      {Opcodes::Divide, [](VM *vm) { vm->Divide(); }},
      {Opcodes::LeftDivide, [](VM *vm) { vm->LeftDivide(); }},
      {Opcodes::RightDivide, [](VM *vm) { vm->RightDivide(); }},
      {Opcodes::ObjectDivide, [](VM *vm) { vm->ObjectDivide(); }},
      {Opcodes::VariableDivideAssign, [](VM *vm) { vm->VariableDivideAssign(); }},
      {Opcodes::VariableRightDivideAssign, [](VM *vm) { vm->VariableRightDivideAssign(); }},
      {Opcodes::VariableObjectDivideAssign, [](VM *vm) { vm->VariableObjectDivideAssign(); }},
      {Opcodes::ElementDivideAssign, [](VM *vm) { vm->ElementDivideAssign(); }},
      {Opcodes::ElementRightDivideAssign, [](VM *vm) { vm->ElementRightDivideAssign(); }},
      {Opcodes::ElementObjectDivideAssign, [](VM *vm) { vm->ElementObjectDivideAssign(); }}};
  opcode_handlers_ = std::vector<OpcodeHandler>(Opcodes::NumReserved, nullptr);
  for (auto const &info : array)
  {
    AddOpcodeHandler(info);
  }
  module->VMSetup(this);
}

bool VM::Execute(std::string &error, Variant &output)
{
  size_t const num_strings = script_->strings.size();
  strings_                 = std::vector<Ptr<String>>(num_strings);
  for (size_t i = 0; i < num_strings; ++i)
  {
    std::string const &str = script_->strings[i];
    strings_[i]            = Ptr<String>(new String(this, str, true));
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
  error.clear();
  do
  {
    instruction_ = &function_->instructions[size_t(pc_)];
    ++pc_;
    OpcodeHandler handler = opcode_handlers_[instruction_->opcode];
    if (handler)
    {
      handler(this);
    }
    else
    {
      RuntimeError("unknown opcode");
      break;
    }
  } while (!stop_);
  strings_.clear();
  bool const ok = error_.empty();
  if (ok)
  {
    if (sp_ == 0)
    {
      // The executed function returned a value, so transfer it to the output
      Variant &result = stack_[sp_--];
      output          = std::move(result);
    }
    // Success
    return true;
  }
  // We've got a runtime error
  // Reset all variables
  for (int i = 0; i < STACK_SIZE; ++i)
  {
    stack_[i].Reset();
  }
  error = error_;
  return false;
}

void VM::RuntimeError(std::string const &message)
{
  std::stringstream stream;
  stream << "runtime error: line " << instruction_->line << ": " << message;
  error_ = stream.str();
  stop_  = true;
}

void VM::Destruct(int scope_number)
{
  // Destruct all live objects in the current frame and with scope >= scope_number
  while (live_object_sp_ >= 0)
  {
    LiveObjectInfo const &info = live_object_stack_[live_object_sp_];
    if ((info.frame_sp != frame_sp_) || (info.scope_number < scope_number))
    {
      break;
    }
    Variant &variable = GetVariable(info.variable_index);
    variable.Reset();
    --live_object_sp_;
  }
}

}  // namespace vm
}  // namespace fetch
