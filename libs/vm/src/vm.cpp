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

#include "vm/module.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace vm {

VM::VM(Module *module)
{
  live_object_stack_.reserve(100);

  FunctionInfoArray function_info_array;

  module->GetDetails(type_info_array_, type_info_map_, registered_types_, function_info_array,
                     deserialization_constructors_, cpp_copy_constructors_);
  auto num_types     = static_cast<uint16_t>(type_info_array_.size());
  auto num_functions = static_cast<uint16_t>(function_info_array.size());
  auto num_opcodes   = static_cast<uint16_t>(Opcodes::NumReserved + num_functions);

  opcode_info_array_ = OpcodeInfoArray(num_opcodes);

  AddOpcodeInfo(Opcodes::LocalVariableDeclare, "LocalVariableDeclare",
                [](VM *vm) { vm->Handler__LocalVariableDeclare(); });
  AddOpcodeInfo(Opcodes::LocalVariableDeclareAssign, "LocalVariableDeclareAssign",
                [](VM *vm) { vm->Handler__LocalVariableDeclareAssign(); });
  AddOpcodeInfo(Opcodes::PushNull, "PushNull", [](VM *vm) { vm->Handler__PushNull(); });
  AddOpcodeInfo(Opcodes::PushFalse, "PushFalse", [](VM *vm) { vm->Handler__PushFalse(); });
  AddOpcodeInfo(Opcodes::PushTrue, "PushTrue", [](VM *vm) { vm->Handler__PushTrue(); });
  AddOpcodeInfo(Opcodes::PushString, "PushString", [](VM *vm) { vm->Handler__PushString(); });
  AddOpcodeInfo(Opcodes::PushConstant, "PushConstant", [](VM *vm) { vm->Handler__PushConstant(); });
  AddOpcodeInfo(Opcodes::PushLocalVariable, "PushLocalVariable",
                [](VM *vm) { vm->Handler__PushLocalVariable(); });
  AddOpcodeInfo(Opcodes::PopToLocalVariable, "PopToLocalVariable",
                [](VM *vm) { vm->Handler__PopToLocalVariable(); });
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
  AddOpcodeInfo(Opcodes::LocalVariablePrefixInc, "LocalVariablePrefixInc",
                [](VM *vm) { vm->Handler__LocalVariablePrefixInc(); });
  AddOpcodeInfo(Opcodes::LocalVariablePrefixDec, "LocalVariablePrefixDec",
                [](VM *vm) { vm->Handler__LocalVariablePrefixDec(); });
  AddOpcodeInfo(Opcodes::LocalVariablePostfixInc, "LocalVariablePostfixInc",
                [](VM *vm) { vm->Handler__LocalVariablePostfixInc(); });
  AddOpcodeInfo(Opcodes::LocalVariablePostfixDec, "LocalVariablePostfixDec",
                [](VM *vm) { vm->Handler__LocalVariablePostfixDec(); });
  AddOpcodeInfo(Opcodes::JumpIfFalseOrPop, "JumpIfFalseOrPop",
                [](VM *vm) { vm->Handler__JumpIfFalseOrPop(); });
  AddOpcodeInfo(Opcodes::JumpIfTrueOrPop, "JumpIfTrueOrPop",
                [](VM *vm) { vm->Handler__JumpIfTrueOrPop(); });
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
  AddOpcodeInfo(Opcodes::LocalVariablePrimitiveInplaceAdd, "LocalVariablePrimitiveInplaceAdd",
                [](VM *vm) { vm->Handler__LocalVariablePrimitiveInplaceAdd(); });
  AddOpcodeInfo(Opcodes::LocalVariableObjectInplaceAdd, "LocalVariableObjectInplaceAdd",
                [](VM *vm) { vm->Handler__LocalVariableObjectInplaceAdd(); });
  AddOpcodeInfo(Opcodes::LocalVariableObjectInplaceRightAdd, "LocalVariableObjectInplaceRightAdd",
                [](VM *vm) { vm->Handler__LocalVariableObjectInplaceRightAdd(); });
  AddOpcodeInfo(Opcodes::PrimitiveSubtract, "PrimitiveSubtract",
                [](VM *vm) { vm->Handler__PrimitiveSubtract(); });
  AddOpcodeInfo(Opcodes::ObjectSubtract, "ObjectSubtract",
                [](VM *vm) { vm->Handler__ObjectSubtract(); });
  AddOpcodeInfo(Opcodes::ObjectLeftSubtract, "ObjectLeftSubtract",
                [](VM *vm) { vm->Handler__ObjectLeftSubtract(); });
  AddOpcodeInfo(Opcodes::ObjectRightSubtract, "ObjectRightSubtract",
                [](VM *vm) { vm->Handler__ObjectRightSubtract(); });
  AddOpcodeInfo(Opcodes::LocalVariablePrimitiveInplaceSubtract,
                "LocalVariablePrimitiveInplaceSubtract",
                [](VM *vm) { vm->Handler__LocalVariablePrimitiveInplaceSubtract(); });
  AddOpcodeInfo(Opcodes::LocalVariableObjectInplaceSubtract, "LocalVariableObjectInplaceSubtract",
                [](VM *vm) { vm->Handler__LocalVariableObjectInplaceSubtract(); });
  AddOpcodeInfo(Opcodes::LocalVariableObjectInplaceRightSubtract,
                "LocalVariableObjectInplaceRightSubtract",
                [](VM *vm) { vm->Handler__LocalVariableObjectInplaceRightSubtract(); });
  AddOpcodeInfo(Opcodes::PrimitiveMultiply, "PrimitiveMultiply",
                [](VM *vm) { vm->Handler__PrimitiveMultiply(); });
  AddOpcodeInfo(Opcodes::ObjectMultiply, "ObjectMultiply",
                [](VM *vm) { vm->Handler__ObjectMultiply(); });
  AddOpcodeInfo(Opcodes::ObjectLeftMultiply, "ObjectLeftMultiply",
                [](VM *vm) { vm->Handler__ObjectLeftMultiply(); });
  AddOpcodeInfo(Opcodes::ObjectRightMultiply, "ObjectRightMultiply",
                [](VM *vm) { vm->Handler__ObjectRightMultiply(); });
  AddOpcodeInfo(Opcodes::LocalVariablePrimitiveInplaceMultiply,
                "LocalVariablePrimitiveInplaceMultiply",
                [](VM *vm) { vm->Handler__LocalVariablePrimitiveInplaceMultiply(); });
  AddOpcodeInfo(Opcodes::LocalVariableObjectInplaceMultiply, "LocalVariableObjectInplaceMultiply",
                [](VM *vm) { vm->Handler__LocalVariableObjectInplaceMultiply(); });
  AddOpcodeInfo(Opcodes::LocalVariableObjectInplaceRightMultiply,
                "LocalVariableObjectInplaceRightMultiply",
                [](VM *vm) { vm->Handler__LocalVariableObjectInplaceRightMultiply(); });
  AddOpcodeInfo(Opcodes::PrimitiveDivide, "PrimitiveDivide",
                [](VM *vm) { vm->Handler__PrimitiveDivide(); });
  AddOpcodeInfo(Opcodes::ObjectDivide, "ObjectDivide", [](VM *vm) { vm->Handler__ObjectDivide(); });
  AddOpcodeInfo(Opcodes::ObjectLeftDivide, "ObjectLeftDivide",
                [](VM *vm) { vm->Handler__ObjectLeftDivide(); });
  AddOpcodeInfo(Opcodes::ObjectRightDivide, "ObjectRightDivide",
                [](VM *vm) { vm->Handler__ObjectRightDivide(); });
  AddOpcodeInfo(Opcodes::LocalVariablePrimitiveInplaceDivide, "LocalVariablePrimitiveInplaceDivide",
                [](VM *vm) { vm->Handler__LocalVariablePrimitiveInplaceDivide(); });
  AddOpcodeInfo(Opcodes::LocalVariableObjectInplaceDivide, "LocalVariableObjectInplaceDivide",
                [](VM *vm) { vm->Handler__LocalVariableObjectInplaceDivide(); });
  AddOpcodeInfo(Opcodes::LocalVariableObjectInplaceRightDivide,
                "LocalVariableObjectInplaceRightDivide",
                [](VM *vm) { vm->Handler__LocalVariableObjectInplaceRightDivide(); });
  AddOpcodeInfo(Opcodes::PrimitiveModulo, "PrimitiveModulo",
                [](VM *vm) { vm->Handler__PrimitiveModulo(); });
  AddOpcodeInfo(Opcodes::LocalVariablePrimitiveInplaceModulo, "LocalVariablePrimitiveInplaceModulo",
                [](VM *vm) { vm->Handler__LocalVariablePrimitiveInplaceModulo(); });
  AddOpcodeInfo(Opcodes::InitialiseArray, "InitialiseArray",
                [](VM *vm) { vm->Handler__InitialiseArray(); });
  AddOpcodeInfo(Opcodes::ContractVariableDeclareAssign, "ContractVariableDeclareAssign",
                [](VM *vm) { vm->Handler__ContractVariableDeclareAssign(); });
  AddOpcodeInfo(Opcodes::InvokeContractFunction, "InvokeContractFunction",
                [](VM *vm) { vm->Handler__InvokeContractFunction(); });
  AddOpcodeInfo(Opcodes::PushLargeConstant, "PushLargeConstant",
                [](VM *vm) { vm->Handler__PushLargeConstant(); });
  AddOpcodeInfo(Opcodes::PushMemberVariable, "PushMemberVariable",
                [](VM *vm) { vm->Handler__PushMemberVariable(); });
  AddOpcodeInfo(Opcodes::PopToMemberVariable, "PopToMemberVariable",
                [](VM *vm) { vm->Handler__PopToMemberVariable(); });
  AddOpcodeInfo(Opcodes::MemberVariablePrefixInc, "MemberVariablePrefixInc",
                [](VM *vm) { vm->Handler__MemberVariablePrefixInc(); });
  AddOpcodeInfo(Opcodes::MemberVariablePrefixDec, "MemberVariablePrefixDec",
                [](VM *vm) { vm->Handler__MemberVariablePrefixDec(); });
  AddOpcodeInfo(Opcodes::MemberVariablePostfixInc, "MemberVariablePostfixInc",
                [](VM *vm) { vm->Handler__MemberVariablePostfixInc(); });
  AddOpcodeInfo(Opcodes::MemberVariablePostfixDec, "MemberVariablePostfixDec",
                [](VM *vm) { vm->Handler__MemberVariablePostfixDec(); });
  AddOpcodeInfo(Opcodes::MemberVariablePrimitiveInplaceAdd, "MemberVariablePrimitiveInplaceAdd",
                [](VM *vm) { vm->Handler__MemberVariablePrimitiveInplaceAdd(); });
  AddOpcodeInfo(Opcodes::MemberVariableObjectInplaceAdd, "MemberVariableObjectInplaceAdd",
                [](VM *vm) { vm->Handler__MemberVariableObjectInplaceAdd(); });
  AddOpcodeInfo(Opcodes::MemberVariableObjectInplaceRightAdd, "MemberVariableObjectInplaceRightAdd",
                [](VM *vm) { vm->Handler__MemberVariableObjectInplaceRightAdd(); });
  AddOpcodeInfo(Opcodes::MemberVariablePrimitiveInplaceSubtract,
                "MemberVariablePrimitiveInplaceSubtract",
                [](VM *vm) { vm->Handler__MemberVariablePrimitiveInplaceSubtract(); });
  AddOpcodeInfo(Opcodes::MemberVariableObjectInplaceSubtract, "MemberVariableObjectInplaceSubtract",
                [](VM *vm) { vm->Handler__MemberVariableObjectInplaceSubtract(); });
  AddOpcodeInfo(Opcodes::MemberVariableObjectInplaceRightSubtract,
                "MemberVariableObjectInplaceRightSubtract",
                [](VM *vm) { vm->Handler__MemberVariableObjectInplaceRightSubtract(); });
  AddOpcodeInfo(Opcodes::MemberVariablePrimitiveInplaceMultiply,
                "MemberVariablePrimitiveInplaceMultiply",
                [](VM *vm) { vm->Handler__MemberVariablePrimitiveInplaceMultiply(); });
  AddOpcodeInfo(Opcodes::MemberVariableObjectInplaceMultiply, "MemberVariableObjectInplaceMultiply",
                [](VM *vm) { vm->Handler__MemberVariableObjectInplaceMultiply(); });
  AddOpcodeInfo(Opcodes::MemberVariableObjectInplaceRightMultiply,
                "MemberVariableObjectInplaceRightMultiply",
                [](VM *vm) { vm->Handler__MemberVariableObjectInplaceRightMultiply(); });
  AddOpcodeInfo(Opcodes::MemberVariablePrimitiveInplaceDivide,
                "MemberVariablePrimitiveInplaceDivide",
                [](VM *vm) { vm->Handler__MemberVariablePrimitiveInplaceDivide(); });
  AddOpcodeInfo(Opcodes::MemberVariableObjectInplaceDivide, "MemberVariableObjectInplaceDivide",
                [](VM *vm) { vm->Handler__MemberVariableObjectInplaceDivide(); });
  AddOpcodeInfo(Opcodes::MemberVariableObjectInplaceRightDivide,
                "MemberVariableObjectInplaceRightDivide",
                [](VM *vm) { vm->Handler__MemberVariableObjectInplaceRightDivide(); });
  AddOpcodeInfo(Opcodes::MemberVariablePrimitiveInplaceModulo,
                "MemberVariablePrimitiveInplaceModulo",
                [](VM *vm) { vm->Handler__MemberVariablePrimitiveInplaceModulo(); });
  AddOpcodeInfo(Opcodes::PushSelf, "PushSelf", [](VM *vm) { vm->Handler__PushSelf(); });
  AddOpcodeInfo(Opcodes::InvokeUserDefinedConstructor, "InvokeUserDefinedConstructor",
                [](VM *vm) { vm->Handler__InvokeUserDefinedConstructor(); });
  AddOpcodeInfo(Opcodes::InvokeUserDefinedMemberFunction, "InvokeUserDefinedMemberFunction",
                [](VM *vm) { vm->Handler__InvokeUserDefinedMemberFunction(); });

  opcode_map_.clear();
  for (uint16_t i = 0; i < num_functions; ++i)
  {
    auto        opcode = static_cast<uint16_t>(Opcodes::NumReserved + i);
    auto const &info   = function_info_array[i];
    AddOpcodeInfo(opcode, info.unique_name, info.handler, info.static_charge);
    opcode_map_[info.unique_name] = opcode;
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
  frame_sp_       = -1;
  bsp_            = 0;
  sp_             = function_->num_variables - 1;
  range_loop_sp_  = -1;
  pc_             = 0;
  instruction_pc_ = 0;
  instruction_    = nullptr;
  current_op_     = nullptr;
  stop_           = false;
  live_object_stack_.clear();
  self_.Reset();
  error_.clear();
  error.clear();
  try
  {
    if (sp_ < STACK_SIZE)
    {
      do
      {
        instruction_pc_ = pc_;
        instruction_    = &function_->instructions[pc_++];

        assert(instruction_->opcode < opcode_info_array_.size());

        current_op_ = &opcode_info_array_[instruction_->opcode];

        if (!current_op_->handler)
        {
          RuntimeError("unknown opcode");
          break;
        }

        IncreaseChargeTotal(current_op_->static_charge);

        if (ChargeLimitExceeded())
        {
          break;
        }

        // execute the handler for the op code
        current_op_->handler(this);

      } while (!stop_);
    }
    else
    {
      RuntimeError("stack overflow");
    }
  }
  catch (std::exception const &e)
  {
    RuntimeError(std::string{"Fatal error: "} + e.what());
    stop_ = true;
  }

  bool const ok = !HasError();

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
  for (auto &variable : stack_)
  {
    variable.Reset();
  }

  // Reset all frames
  for (auto &frame : frame_stack_)
  {
    frame.self.Reset();
  }

  self_.Reset();

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
  while (!live_object_stack_.empty())
  {
    LiveObjectInfo const &info = live_object_stack_.back();
    if ((info.frame_sp != frame_sp_) || (info.scope_number < scope_number))
    {
      break;
    }
    Variant &variable = GetLocalVariable(info.variable_index);
    variable.Reset();
    live_object_stack_.pop_back();
  }
}

ChargeAmount VM::GetChargeTotal() const
{
  return charge_total_;
}

void VM::IncreaseChargeTotal(ChargeAmount const amount)
{
  ChargeAmount const adjusted_amount = amount == 0 ? 1u : amount;

  // if charge total would overflow, set it to max
  if ((std::numeric_limits<ChargeAmount>::max() - charge_total_) < adjusted_amount)
  {
    charge_total_ = std::numeric_limits<ChargeAmount>::max();
  }
  else
  {
    charge_total_ += adjusted_amount;
  }
}

ChargeAmount VM::GetChargeLimit() const
{
  return charge_limit_;
}

bool VM::ChargeLimitExceeded()
{
  if ((charge_limit_ != 0u) && (charge_total_ >= charge_limit_))
  {
    RuntimeError("Charge limit reached");

    return true;
  }

  return false;
}

void VM::SetChargeLimit(ChargeAmount limit)
{
  charge_limit_ = limit;
}

void VM::UpdateCharges(std::unordered_map<std::string, ChargeAmount> const &opcode_static_charges)
{
  for (auto const &entry : opcode_static_charges)
  {
    auto const &unique_name = entry.first;

    auto it =
        std::find_if(opcode_info_array_.begin(), opcode_info_array_.end(),
                     [&unique_name](OpcodeInfo &info) { return info.unique_name == unique_name; });

    if (it != opcode_info_array_.end())
    {
      it->static_charge = entry.second;
    }
  }
}

}  // namespace vm
}  // namespace fetch
