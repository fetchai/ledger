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

namespace fetch {
namespace vm {

VM::VM(Module *module)
{
  FunctionInfoArray function_info_array;
  module->GetDetails(type_info_array_, type_info_map_, registered_types_, function_info_array);
  uint16_t num_types     = uint16_t(type_info_array_.size());
  uint16_t num_functions = uint16_t(function_info_array.size());
  uint16_t num_opcodes   = uint16_t(Opcodes::NumReserved + num_functions);
  opcode_info_array_     = OpcodeInfoArray(num_opcodes);

  AddOpcodeInfo(Opcodes::VariableDeclare, "VariableDeclare",
                [](VM *vm) { vm->Handler__VariableDeclare(); });
  AddOpcodeInfo(Opcodes::VariableDeclareAssign, "VariableDeclareAssign",
                [](VM *vm) { vm->Handler__VariableDeclareAssign(); });
  AddOpcodeInfo(Opcodes::PushNull, "PushNull", [](VM *vm) { vm->Handler__PushNull(); });
  AddOpcodeInfo(Opcodes::PushFalse, "PushFalse", [](VM *vm) { vm->Handler__PushFalse(); });
  AddOpcodeInfo(Opcodes::PushTrue, "PushTrue", [](VM *vm) { vm->Handler__PushTrue(); });
  AddOpcodeInfo(Opcodes::PushString, "PushString", [](VM *vm) { vm->Handler__PushString(); });
  AddOpcodeInfo(Opcodes::PushConstant, "PushConstant", [](VM *vm) { vm->Handler__PushConstant(); });
  AddOpcodeInfo(Opcodes::PushVariable, "PushVariable", [](VM *vm) { vm->Handler__PushVariable(); });
  AddOpcodeInfo(Opcodes::PopToVariable, "PopToVariable",
                [](VM *vm) { vm->Handler__PopToVariable(); });
  AddOpcodeInfo(Opcodes::Inc, "Inc", [](VM *vm) { vm->Handler__Inc(); });
  AddOpcodeInfo(Opcodes::Dec, "Dec", [](VM *vm) { vm->Handler__Dec(); });
  AddOpcodeInfo(Opcodes::Duplicate, "Duplicate", [](VM *vm) { vm->Handler__Duplicate(); });
  AddOpcodeInfo(Opcodes::DuplicateInsert, "DuplicateInsert",
                [](VM *vm) { vm->Handler__DuplicateInsert(); });
  AddOpcodeInfo(Opcodes::Discard, "Discard", [](VM *vm) { vm->Handler__Discard(); });
  AddOpcodeInfo(Opcodes::Destruct, "Destruct", [](VM *vm) { vm->Handler__Destruct(); });
  AddOpcodeInfo(Opcodes::Break, "Break", [](VM *vm) { vm->Handler__Break(); });
  AddOpcodeInfo(Opcodes::Continue, "Continue", [](VM *vm) { vm->Handler__Continue(); });
  AddOpcodeInfo(Opcodes::Jump, "Jump", [](VM *vm) { vm->Handler__Jump(); });
  AddOpcodeInfo(Opcodes::JumpIfFalse, "JumpIfFalse", [](VM *vm) { vm->Handler__JumpIfFalse(); });
  AddOpcodeInfo(Opcodes::JumpIfTrue, "JumpIfTrue", [](VM *vm) { vm->Handler__JumpIfTrue(); });
  AddOpcodeInfo(Opcodes::Return, "Return", [](VM *vm) { vm->Handler__Return(); });
  AddOpcodeInfo(Opcodes::ReturnValue, "ReturnValue", [](VM *vm) { vm->Handler__Return(); });
  AddOpcodeInfo(Opcodes::ForRangeInit, "ForRangeInit", [](VM *vm) { vm->Handler__ForRangeInit(); });
  AddOpcodeInfo(Opcodes::ForRangeIterate, "ForRangeIterate",
                [](VM *vm) { vm->Handler__ForRangeIterate(); });
  AddOpcodeInfo(Opcodes::ForRangeTerminate, "ForRangeTerminate",
                [](VM *vm) { vm->Handler__ForRangeTerminate(); });
  AddOpcodeInfo(Opcodes::InvokeUserDefinedFreeFunction, "InvokeUserDefinedFreeFunction",
                [](VM *vm) { vm->Handler__InvokeUserDefinedFreeFunction(); });
  AddOpcodeInfo(Opcodes::VariablePrefixInc, "VariablePrefixInc",
                [](VM *vm) { vm->Handler__VariablePrefixInc(); });
  AddOpcodeInfo(Opcodes::VariablePrefixDec, "VariablePrefixDec",
                [](VM *vm) { vm->Handler__VariablePrefixDec(); });
  AddOpcodeInfo(Opcodes::VariablePostfixInc, "VariablePostfixInc",
                [](VM *vm) { vm->Handler__VariablePostfixInc(); });
  AddOpcodeInfo(Opcodes::VariablePostfixDec, "VariablePostfixDec",
                [](VM *vm) { vm->Handler__VariablePostfixDec(); });
  AddOpcodeInfo(Opcodes::And, "And", [](VM *vm) { vm->Handler__And(); });
  AddOpcodeInfo(Opcodes::Or, "Or", [](VM *vm) { vm->Handler__Or(); });
  AddOpcodeInfo(Opcodes::Not, "Not", [](VM *vm) { vm->Handler__Not(); });
  AddOpcodeInfo(Opcodes::PrimitiveEqual, "PrimitiveEqual",
                [](VM *vm) { vm->Handler__PrimitiveEqual(); });
  AddOpcodeInfo(Opcodes::ObjectEqual, "ObjectEqual", [](VM *vm) { vm->Handler__ObjectEqual(); });
  AddOpcodeInfo(Opcodes::PrimitiveNotEqual, "PrimitiveNotEqual",
                [](VM *vm) { vm->Handler__PrimitiveNotEqual(); });
  AddOpcodeInfo(Opcodes::ObjectNotEqual, "ObjectNotEqual",
                [](VM *vm) { vm->Handler__ObjectNotEqual(); });
  AddOpcodeInfo(Opcodes::PrimitiveLessThan, "PrimitiveLessThan",
                [](VM *vm) { vm->Handler__PrimitiveLessThan(); });
  AddOpcodeInfo(Opcodes::ObjectLessThan, "ObjectLessThan",
                [](VM *vm) { vm->Handler__ObjectLessThan(); });
  AddOpcodeInfo(Opcodes::PrimitiveLessThanOrEqual, "PrimitiveLessThanOrEqual",
                [](VM *vm) { vm->Handler__PrimitiveLessThanOrEqual(); });
  AddOpcodeInfo(Opcodes::ObjectLessThanOrEqual, "ObjectLessThanOrEqual",
                [](VM *vm) { vm->Handler__ObjectLessThanOrEqual(); });
  AddOpcodeInfo(Opcodes::PrimitiveGreaterThan, "PrimitiveGreaterThan",
                [](VM *vm) { vm->Handler__PrimitiveGreaterThan(); });
  AddOpcodeInfo(Opcodes::ObjectGreaterThan, "ObjectGreaterThan",
                [](VM *vm) { vm->Handler__ObjectGreaterThan(); });
  AddOpcodeInfo(Opcodes::PrimitiveGreaterThanOrEqual, "PrimitiveGreaterThanOrEqual",
                [](VM *vm) { vm->Handler__PrimitiveGreaterThanOrEqual(); });
  AddOpcodeInfo(Opcodes::ObjectGreaterThanOrEqual, "ObjectGreaterThanOrEqual",
                [](VM *vm) { vm->Handler__ObjectGreaterThanOrEqual(); });
  AddOpcodeInfo(Opcodes::PrimitiveNegate, "PrimitiveNegate",
                [](VM *vm) { vm->Handler__PrimitiveNegate(); });
  AddOpcodeInfo(Opcodes::ObjectNegate, "ObjectNegate", [](VM *vm) { vm->Handler__ObjectNegate(); });
  AddOpcodeInfo(Opcodes::PrimitiveAdd, "PrimitiveAdd", [](VM *vm) { vm->Handler__PrimitiveAdd(); });
  AddOpcodeInfo(Opcodes::ObjectAdd, "ObjectAdd", [](VM *vm) { vm->Handler__ObjectAdd(); });
  AddOpcodeInfo(Opcodes::ObjectLeftAdd, "ObjectLeftAdd",
                [](VM *vm) { vm->Handler__ObjectLeftAdd(); });
  AddOpcodeInfo(Opcodes::ObjectRightAdd, "ObjectRightAdd",
                [](VM *vm) { vm->Handler__ObjectRightAdd(); });
  AddOpcodeInfo(Opcodes::VariablePrimitiveInplaceAdd, "VariablePrimitiveInplaceAdd",
                [](VM *vm) { vm->Handler__VariablePrimitiveInplaceAdd(); });
  AddOpcodeInfo(Opcodes::VariableObjectInplaceAdd, "VariableObjectInplaceAdd",
                [](VM *vm) { vm->Handler__VariableObjectInplaceAdd(); });
  AddOpcodeInfo(Opcodes::VariableObjectInplaceRightAdd, "VariableObjectInplaceRightAdd",
                [](VM *vm) { vm->Handler__VariableObjectInplaceRightAdd(); });
  AddOpcodeInfo(Opcodes::PrimitiveSubtract, "PrimitiveSubtract",
                [](VM *vm) { vm->Handler__PrimitiveSubtract(); });
  AddOpcodeInfo(Opcodes::ObjectSubtract, "ObjectSubtract",
                [](VM *vm) { vm->Handler__ObjectSubtract(); });
  AddOpcodeInfo(Opcodes::ObjectLeftSubtract, "ObjectLeftSubtract",
                [](VM *vm) { vm->Handler__ObjectLeftSubtract(); });
  AddOpcodeInfo(Opcodes::ObjectRightSubtract, "ObjectRightSubtract",
                [](VM *vm) { vm->Handler__ObjectRightSubtract(); });
  AddOpcodeInfo(Opcodes::VariablePrimitiveInplaceSubtract, "VariablePrimitiveInplaceSubtract",
                [](VM *vm) { vm->Handler__VariablePrimitiveInplaceSubtract(); });
  AddOpcodeInfo(Opcodes::VariableObjectInplaceSubtract, "VariableObjectInplaceSubtract",
                [](VM *vm) { vm->Handler__VariableObjectInplaceSubtract(); });
  AddOpcodeInfo(Opcodes::VariableObjectInplaceRightSubtract, "VariableObjectInplaceRightSubtract",
                [](VM *vm) { vm->Handler__VariableObjectInplaceRightSubtract(); });
  AddOpcodeInfo(Opcodes::PrimitiveMultiply, "PrimitiveMultiply",
                [](VM *vm) { vm->Handler__PrimitiveMultiply(); });
  AddOpcodeInfo(Opcodes::ObjectMultiply, "ObjectMultiply",
                [](VM *vm) { vm->Handler__ObjectMultiply(); });
  AddOpcodeInfo(Opcodes::ObjectLeftMultiply, "ObjectLeftMultiply",
                [](VM *vm) { vm->Handler__ObjectLeftMultiply(); });
  AddOpcodeInfo(Opcodes::ObjectRightMultiply, "ObjectRightMultiply",
                [](VM *vm) { vm->Handler__ObjectRightMultiply(); });
  AddOpcodeInfo(Opcodes::VariablePrimitiveInplaceMultiply, "VariablePrimitiveInplaceMultiply",
                [](VM *vm) { vm->Handler__VariablePrimitiveInplaceMultiply(); });
  AddOpcodeInfo(Opcodes::VariableObjectInplaceMultiply, "VariableObjectInplaceMultiply",
                [](VM *vm) { vm->Handler__VariableObjectInplaceMultiply(); });
  AddOpcodeInfo(Opcodes::VariableObjectInplaceRightMultiply, "VariableObjectInplaceRightMultiply",
                [](VM *vm) { vm->Handler__VariableObjectInplaceRightMultiply(); });
  AddOpcodeInfo(Opcodes::PrimitiveDivide, "PrimitiveDivide",
                [](VM *vm) { vm->Handler__PrimitiveDivide(); });
  AddOpcodeInfo(Opcodes::ObjectDivide, "ObjectDivide", [](VM *vm) { vm->Handler__ObjectDivide(); });
  AddOpcodeInfo(Opcodes::ObjectLeftDivide, "ObjectLeftDivide",
                [](VM *vm) { vm->Handler__ObjectLeftDivide(); });
  AddOpcodeInfo(Opcodes::ObjectRightDivide, "ObjectRightDivide",
                [](VM *vm) { vm->Handler__ObjectRightDivide(); });
  AddOpcodeInfo(Opcodes::VariablePrimitiveInplaceDivide, "VariablePrimitiveInplaceDivide",
                [](VM *vm) { vm->Handler__VariablePrimitiveInplaceDivide(); });
  AddOpcodeInfo(Opcodes::VariableObjectInplaceDivide, "VariableObjectInplaceDivide",
                [](VM *vm) { vm->Handler__VariableObjectInplaceDivide(); });
  AddOpcodeInfo(Opcodes::VariableObjectInplaceRightDivide, "VariableObjectInplaceRightDivide",
                [](VM *vm) { vm->Handler__VariableObjectInplaceRightDivide(); });
  AddOpcodeInfo(Opcodes::PrimitiveModulo, "PrimitiveModulo",
                [](VM *vm) { vm->Handler__PrimitiveModulo(); });
  AddOpcodeInfo(Opcodes::VariablePrimitiveInplaceModulo, "VariablePrimitiveInplaceModulo",
                [](VM *vm) { vm->Handler__VariablePrimitiveInplaceModulo(); });

  opcode_map_.clear();
  for (uint16_t i = 0; i < num_functions; ++i)
  {
    uint16_t    opcode = uint16_t(Opcodes::NumReserved + i);
    auto const &info   = function_info_array[i];
    AddOpcodeInfo(opcode, info.unique_id, info.handler);
    opcode_map_[info.unique_id] = opcode;
  }

  generator_.Initialise(this, num_types);
}

bool VM::GenerateExecutable(IR const &ir, std::string const &name, Executable &executable,
                            std::vector<std::string> &errors)
{
  return generator_.GenerateExecutable(ir, name, executable, errors);
}

bool VM::Execute(std::string &error, Variant &output)
{
  size_t const num_strings = executable_->strings.size();
  strings_                 = std::vector<Ptr<String>>(num_strings);
  for (size_t i = 0; i < num_strings; ++i)
  {
    std::string const &str = executable_->strings[i];
    strings_[i]            = Ptr<String>(new String(this, str, true));
  }

  size_t const num_local_types = executable_->types.size();
  for (size_t i = 0; i < num_local_types; ++i)
  {
    TypeInfo const &type_info = executable_->types[i];
    type_info_array_.push_back(type_info);
  }

  frame_sp_       = -1;
  bsp_            = 0;
  sp_             = function_->num_variables - 1;
  range_loop_sp_  = -1;
  live_object_sp_ = -1;
  pc_             = 0;
  instruction_pc_ = 0;
  instruction_    = nullptr;
  stop_           = false;
  error_.clear();
  error.clear();

  do
  {
    instruction_pc_  = pc_;
    instruction_     = &function_->instructions[pc_++];
    OpcodeInfo &info = opcode_info_array_[instruction_->opcode];
    if (info.handler)
    {
      info.handler(this);
    }
    else
    {
      RuntimeError("unknown opcode");
      break;
    }
  } while (!stop_);

  bool const ok = error_.empty();

  // Remove the executable's strings
  strings_.clear();

  // Remove the executable's local types
  for (size_t i = 0; i < num_local_types; ++i)
  {
    type_info_array_.pop_back();
  }

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
  for (auto & variable : stack_)
  {
    variable.Reset();
  }
  error = error_;
  return false;
}

void VM::RuntimeError(std::string const &message)
{
  uint16_t const    line = function_->FindLineNumber(instruction_pc_);
  std::stringstream stream;
  stream << "runtime error: line " << line << ": " << message;
  error_ = stream.str();
  stop_  = true;
}

void VM::Destruct(uint16_t scope_number)
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
