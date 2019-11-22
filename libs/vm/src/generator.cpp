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

#include "vm/generator.hpp"
#include "vm/vm.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace vm {

void Generator::Initialise(VM *vm, uint16_t num_system_types)
{
  vm_               = vm;
  num_system_types_ = num_system_types;
}

bool Generator::GenerateExecutable(IR const &ir, std::string const &executable_name,
                                   Executable &executable, std::vector<std::string> &errors)
{
  executable_ = Executable(executable_name);
  scopes_.clear();
  loops_.clear();
  strings_map_.clear();
  constants_map_.clear();
  large_constants_map_.clear();
  function_ = nullptr;
  line_to_pc_map_.clear();
  errors_.clear();
  errors.clear();

  if (vm_ == nullptr)
  {
    errors.emplace_back("error: Generator is not initialised");
    return false;
  }

  if (!ir.root_)
  {
    errors.emplace_back("error: IR is not initialised");
    return false;
  }

  ResolveTypes(ir);
  ResolveFunctions(ir);

  if (!errors_.empty())
  {
    errors = std::move(errors_);
    return false;
  }

  CreateUserDefinedContracts(ir.root_);
  CreateUserDefinedFunctions(ir.root_);
  HandleBlock(ir.root_);

  executable = executable_;
  scopes_.clear();
  loops_.clear();
  strings_map_.clear();
  constants_map_.clear();
  large_constants_map_.clear();
  function_ = nullptr;
  line_to_pc_map_.clear();
  return true;
}

void Generator::AddLineNumber(uint16_t line, uint16_t pc)
{
  // Store the lowest PC ecountered for each unique line that has instructions associated with it
  auto it = line_to_pc_map_.find(line);
  if (it != line_to_pc_map_.end())
  {
    uint16_t current_pc = it->second;
    if (pc < current_pc)
    {
      it->second = pc;
    }
  }
  else
  {
    line_to_pc_map_[line] = pc;
  }
}

void Generator::ResolveTypes(IR const &ir)
{
  // NOTE: Types in the types_ array are stored depth first
  for (auto const &type : ir.types_)
  {
    if (type->type_kind == TypeKind::UserDefinedContract)
    {
      continue;
    }
    if (type->type_kind == TypeKind::UserDefinedTemplateInstantiation)
    {
      TypeId      template_type_id = type->template_type->id;
      TypeIdArray template_parameter_type_ids;
      for (auto const &template_parameter_type : type->template_parameter_types)
      {
        template_parameter_type_ids.push_back(template_parameter_type->id);
      }
      auto num_local_types = static_cast<uint16_t>(executable_.types.size());
      auto type_id         = uint16_t(num_system_types_ + num_local_types);
      type->id             = type_id;
      TypeInfo type_info(type->type_kind, type->name, type_id, template_type_id,
                         template_parameter_type_ids);
      executable_.AddTypeInfo(std::move(type_info));
      continue;
    }
    uint16_t type_id = vm_->FindType(type->name);
    if (type_id == TypeIds::Unknown)
    {
      errors_.push_back("error: unable to find type '" + type->name + "'");
      continue;
    }
    type->id = type_id;
  }
}

void Generator::ResolveFunctions(IR const &ir)
{
  for (auto const &function : ir.functions_)
  {
    if ((function->function_kind == FunctionKind::UserDefinedFreeFunction) ||
        (function->function_kind == FunctionKind::UserDefinedContractFunction))
    {
      continue;
    }
    uint16_t opcode = vm_->FindOpcode(function->unique_name);
    if (opcode == Opcodes::Unknown)
    {
      errors_.push_back("error: unable to find function '" + function->unique_name + "'");
      continue;
    }
    function->id = opcode;
  }
}

void Generator::CreateUserDefinedContracts(IRBlockNodePtr const &block_node)
{
  for (IRNodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      CreateUserDefinedContracts(ConvertToIRBlockNodePtr(child));
      break;
    }
    case NodeKind::ContractDefinition:
    {
      IRBlockNodePtr      contract_definition_node = ConvertToIRBlockNodePtr(child);
      IRExpressionNodePtr contract_name_node =
          ConvertToIRExpressionNodePtr(contract_definition_node->children[0]);
      std::string const &  contract_name = contract_name_node->text;
      Executable::Contract exe_contract(contract_name);
      for (IRNodePtr const &contract_function_prototype_node :
           contract_definition_node->block_children)
      {
        IRNodePtr       annotations_node = contract_function_prototype_node->children[0];
        AnnotationArray annotations;
        CreateAnnotations(annotations_node, annotations);
        IRExpressionNodePtr function_name_node =
            ConvertToIRExpressionNodePtr(contract_function_prototype_node->children[1]);
        IRFunctionPtr        function       = function_name_node->function;
        std::string const &  function_name  = function->name;
        uint16_t             return_type_id = function->return_type->id;
        Executable::Function exe_function(function_name, annotations, return_type_id);
        for (IRVariablePtr const &variable : function->parameter_variables)
        {
          uint16_t variable_type_id = variable->type->id;
          exe_function.AddParameter(variable->name, variable_type_id);
        }
        function->id = exe_contract.AddFunction(exe_function);
      }
      contract_name_node->type->id = executable_.AddContract(exe_contract);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }
}

void Generator::CreateUserDefinedFunctions(IRBlockNodePtr const &block_node)
{
  for (IRNodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      CreateUserDefinedFunctions(ConvertToIRBlockNodePtr(child));
      break;
    }
    case NodeKind::FunctionDefinition:
    {
      IRBlockNodePtr  function_definition_node = ConvertToIRBlockNodePtr(child);
      IRNodePtr       annotations_node         = function_definition_node->children[0];
      AnnotationArray annotations;
      CreateAnnotations(annotations_node, annotations);
      IRExpressionNodePtr function_name_node =
          ConvertToIRExpressionNodePtr(function_definition_node->children[1]);
      IRFunctionPtr        function       = function_name_node->function;
      std::string const &  function_name  = function->name;
      uint16_t             return_type_id = function->return_type->id;
      Executable::Function exe_function(function_name, annotations, return_type_id);
      for (IRVariablePtr const &variable : function->parameter_variables)
      {
        uint16_t variable_type_id = variable->type->id;
        exe_function.AddParameter(variable->name, variable_type_id);
        variable->id = exe_function.AddVariable(variable->name, variable_type_id, 0);
      }
      function->id = executable_.AddFunction(exe_function);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }
}

void Generator::CreateAnnotations(IRNodePtr const &node, AnnotationArray &annotations)
{
  annotations.clear();
  if (!node)
  {
    return;
  }
  for (IRNodePtr const &annotation_node : node->children)
  {
    Annotation annotation;
    annotation.name = annotation_node->text;
    for (IRNodePtr const &annotation_element_node : annotation_node->children)
    {
      AnnotationElement element;
      if (annotation_element_node->node_kind == NodeKind::AnnotationNameValuePair)
      {
        element.type = AnnotationElementType::NameValuePair;
        SetAnnotationLiteral(annotation_element_node->children[0], element.name);
        SetAnnotationLiteral(annotation_element_node->children[1], element.value);
      }
      else
      {
        element.type = AnnotationElementType::Value;
        SetAnnotationLiteral(annotation_element_node, element.value);
      }
      annotation.elements.push_back(element);
    }
    annotations.push_back(annotation);
  }
}

void Generator::SetAnnotationLiteral(IRNodePtr const &node, AnnotationLiteral &literal)
{
  std::string const &text = node->text;
  switch (node->node_kind)
  {
  case NodeKind::True:
  {
    literal.SetBoolean(true);
    break;
  }
  case NodeKind::False:
  {
    literal.SetBoolean(false);
    break;
  }
  case NodeKind::Integer64:
  {
    auto i = static_cast<int64_t>(std::atoll(text.c_str()));
    literal.SetInteger(i);
    break;
  }
  case NodeKind::Float64:
  {
    double r = std::atof(text.c_str());
    literal.SetReal(r);
    break;
  }
  case NodeKind::String:
  {
    std::string s = text.substr(1, text.size() - 2);
    literal.SetString(s);
    break;
  }
  case NodeKind::Identifier:
  {
    literal.SetIdentifier(text);
    break;
  }
  default:
  {
    assert(false);
    break;
  }
  }  // switch
}

void Generator::HandleBlock(IRBlockNodePtr const &block_node)
{
  for (IRNodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      HandleFile(ConvertToIRBlockNodePtr(child));
      break;
    }
    case NodeKind::PersistentStatement:
    case NodeKind::ContractDefinition:
    {
      // nothing to do here
      break;
    }
    case NodeKind::FunctionDefinition:
    {
      HandleFunctionDefinition(ConvertToIRBlockNodePtr(child));
      break;
    }
    case NodeKind::WhileStatement:
    {
      HandleWhileStatement(ConvertToIRBlockNodePtr(child));
      break;
    }
    case NodeKind::ForStatement:
    {
      HandleForStatement(ConvertToIRBlockNodePtr(child));
      break;
    }
    case NodeKind::IfStatement:
    {
      HandleIfStatement(child);
      break;
    }
    case NodeKind::UseStatement:
    {
      HandleUseStatement(child);
      break;
    }
    case NodeKind::UseAnyStatement:
    {
      HandleUseAnyStatement(child);
      break;
    }
    case NodeKind::ContractStatement:
    {
      HandleContractStatement(child);
      break;
    }
    case NodeKind::VarDeclarationStatement:
    case NodeKind::VarDeclarationTypedAssignmentStatement:
    case NodeKind::VarDeclarationTypelessAssignmentStatement:
    {
      HandleVarStatement(child);
      break;
    }
    case NodeKind::ReturnStatement:
    {
      HandleReturnStatement(child);
      break;
    }
    case NodeKind::BreakStatement:
    {
      HandleBreakStatement(child);
      break;
    }
    case NodeKind::ContinueStatement:
    {
      HandleContinueStatement(child);
      break;
    }
    case NodeKind::Assign:
    {
      HandleAssignmentStatement(ConvertToIRExpressionNodePtr(child));
      break;
    }
    case NodeKind::InplaceAdd:
    case NodeKind::InplaceSubtract:
    case NodeKind::InplaceMultiply:
    case NodeKind::InplaceDivide:
    case NodeKind::InplaceModulo:
    {
      HandleInplaceAssignmentStatement(ConvertToIRExpressionNodePtr(child));
      break;
    }
    default:
    {
      IRExpressionNodePtr expression = ConvertToIRExpressionNodePtr(child);
      HandleExpression(expression);
      if (!expression->type->IsVoid())
      {
        // The result of the expression is not consumed, so issue an
        // instruction that will pop it and throw it away
        Executable::Instruction instruction(Opcodes::Discard);
        instruction.type_id = expression->type->id;
        uint16_t pc         = function_->AddInstruction(instruction);
        AddLineNumber(expression->line, pc);
      }
      break;
    }
    }  // switch
  }
}

void Generator::HandleFile(IRBlockNodePtr const &block_node)
{
  HandleBlock(block_node);
}

void Generator::HandleFunctionDefinition(IRBlockNodePtr const &block_node)
{
  IRExpressionNodePtr function_name_node = ConvertToIRExpressionNodePtr(block_node->children[1]);
  IRFunctionPtr       function           = function_name_node->function;
  function_                              = &(executable_.functions[function->id]);
  line_to_pc_map_.clear();

  ScopeEnter();
  HandleBlock(block_node);
  ScopeLeave(block_node);

  if (function->return_type->IsVoid())
  {
    Executable::Instruction instruction(Opcodes::Return);
    uint16_t                pc = function_->AddInstruction(instruction);
    AddLineNumber(block_node->block_terminator_line, pc);
  }

  for (auto const &it : line_to_pc_map_)
  {
    uint16_t line                 = it.first;
    uint16_t pc                   = it.second;
    function_->pc_to_line_map[pc] = line;
  }

  function_ = nullptr;
  line_to_pc_map_.clear();
}

void Generator::HandleWhileStatement(IRBlockNodePtr const &block_node)
{
  auto const          condition_pc   = uint16_t(function_->instructions.size());
  IRExpressionNodePtr condition_node = ConvertToIRExpressionNodePtr(block_node->children[0]);
  Chain               chain          = HandleConditionExpression(block_node, condition_node);

  auto const              jf_pc = uint16_t(function_->instructions.size());
  Executable::Instruction jf_instruction(Opcodes::JumpIfFalse);
  jf_instruction.index = 0;  // pc placeholder
  function_->AddInstruction(jf_instruction);
  AddLineNumber(condition_node->line, jf_pc);

  auto const while_block_start_pc = uint16_t(function_->instructions.size());
  if (chain.kind == NodeKind::Or)
  {
    FinaliseShortCircuitChain(chain, true, while_block_start_pc);
  }

  ScopeEnter();

  auto const scope_number = uint16_t(scopes_.size() - 1);

  loops_.emplace_back();
  loops_.back().scope_number = scope_number;

  HandleBlock(block_node);
  ScopeLeave(block_node);

  Executable::Instruction jump_instruction(Opcodes::Jump);

  jump_instruction.index = condition_pc;
  {
    uint16_t jump_pc = function_->AddInstruction(jump_instruction);
    AddLineNumber(block_node->block_terminator_line, jump_pc);
  }

  auto const endwhile_pc               = uint16_t(function_->instructions.size());
  function_->instructions[jf_pc].index = endwhile_pc;
  if (chain.kind == NodeKind::And)
  {
    FinaliseShortCircuitChain(chain, true, endwhile_pc);
  }

  Loop &loop = loops_.back();

  for (auto const &jump_pc : loop.continue_pcs)
  {
    function_->instructions[jump_pc].index = condition_pc;
  }

  for (auto const &jump_pc : loop.break_pcs)
  {
    function_->instructions[jump_pc].index = endwhile_pc;
  }

  loops_.pop_back();
}

void Generator::HandleForStatement(IRBlockNodePtr const &block_node)
{
  IRExpressionNodePtr variable_node = ConvertToIRExpressionNodePtr(block_node->children[0]);
  IRVariablePtr       variable      = variable_node->variable;
  auto const          arity         = uint16_t(block_node->children.size() - 1);
  uint16_t            type_id       = variable->type->id;

  for (uint16_t i = 1; i <= arity; ++i)
  {
    HandleExpression(ConvertToIRExpressionNodePtr(block_node->children[i]));
  }

  ScopeEnter();

  auto const scope_number = uint16_t(scopes_.size() - 1);

  // Add the for-loop variable
  variable->id = function_->AddVariable(variable->name, type_id, scope_number);

  Executable::Instruction init_instruction(Opcodes::ForRangeInit);
  init_instruction.type_id = type_id;
  init_instruction.index   = variable->id;  // frame-relative index of the for-loop variable
  init_instruction.data    = arity;         // 2 or 3 range elements

  uint16_t init_pc = function_->AddInstruction(init_instruction);
  AddLineNumber(block_node->line, init_pc);

  Executable::Instruction iterate_instruction(Opcodes::ForRangeIterate);
  iterate_instruction.index = 0;      // pc placeholder
  iterate_instruction.data  = arity;  // 2 or 3 range elements

  uint16_t const iterate_pc = function_->AddInstruction(iterate_instruction);
  AddLineNumber(block_node->line, iterate_pc);

  // for future non-primitive for-loop variables, will need to think about destruction at end of for
  // block break/continue/return inside for loop? continue requires loop variable to retain value?
  // maybe use an extra scope for just the for-loop variable

  loops_.emplace_back();
  loops_.back().scope_number = scope_number;

  HandleBlock(block_node);
  ScopeLeave(block_node);

  Executable::Instruction jump_instruction(Opcodes::Jump);
  jump_instruction.index = iterate_pc;
  {
    uint16_t jump_pc = function_->AddInstruction(jump_instruction);
    AddLineNumber(block_node->block_terminator_line, jump_pc);
  }

  Executable::Instruction terminate_instruction(Opcodes::ForRangeTerminate);
  terminate_instruction.type_id = type_id;
  terminate_instruction.index   = variable->id;

  uint16_t const terminate_pc = function_->AddInstruction(terminate_instruction);
  AddLineNumber(block_node->block_terminator_line, terminate_pc);

  Loop &loop = loops_.back();

  for (auto jump_pc : loop.continue_pcs)
  {
    function_->instructions[jump_pc].index = iterate_pc;
  }

  for (auto jump_pc : loop.break_pcs)
  {
    function_->instructions[jump_pc].index = terminate_pc;
  }

  function_->instructions[iterate_pc].index = terminate_pc;

  loops_.pop_back();
}

void Generator::HandleIfStatement(IRNodePtr const &node)
{
  Chain                 chain;
  auto                  jf_pc = uint16_t(-1);
  std::vector<uint16_t> jump_pcs;
  int const             last_index = static_cast<int>(node->children.size()) - 1;

  for (int i = 0; i <= last_index; ++i)
  {
    IRBlockNodePtr block_node = ConvertToIRBlockNodePtr(node->children[std::size_t(i)]);
    if (block_node->node_kind != NodeKind::Else)
    {
      //
      // The "if" block or one of the "elseif" blocks
      //

      auto const condition_pc = uint16_t(function_->instructions.size());

      if (jf_pc != uint16_t(-1))
      {
        // The previous condition, if there was one, jumps here
        function_->instructions[jf_pc].index = condition_pc;
        if (chain.kind == NodeKind::And)
        {
          FinaliseShortCircuitChain(chain, true, condition_pc);
        }
      }

      IRExpressionNodePtr condition_node = ConvertToIRExpressionNodePtr(block_node->children[0]);
      chain                              = HandleConditionExpression(block_node, condition_node);

      Executable::Instruction jf_instruction(Opcodes::JumpIfFalse);
      jf_instruction.index = 0;  // pc placeholder
      jf_pc                = function_->AddInstruction(jf_instruction);
      AddLineNumber(condition_node->line, jf_pc);

      auto const block_start_pc = uint16_t(function_->instructions.size());
      if (chain.kind == NodeKind::Or)
      {
        FinaliseShortCircuitChain(chain, true, block_start_pc);
      }

      ScopeEnter();
      HandleBlock(block_node);
      ScopeLeave(block_node);

      if (i < last_index)
      {
        // Only arrange jump to the endif if not the last block
        Executable::Instruction jump_instruction(Opcodes::Jump);
        jump_instruction.index = 0;  // pc placeholder
        uint16_t const jump_pc = function_->AddInstruction(jump_instruction);
        AddLineNumber(block_node->block_terminator_line, jump_pc);
        jump_pcs.push_back(jump_pc);
      }
    }
    else
    {
      //
      // The "else" block
      //

      auto const else_block_start_pc = uint16_t(function_->instructions.size());

      // The previous condition jumps here
      function_->instructions[jf_pc].index = else_block_start_pc;
      if (chain.kind == NodeKind::And)
      {
        FinaliseShortCircuitChain(chain, true, else_block_start_pc);
      }

      jf_pc = uint16_t(-1);
      chain = Chain();

      ScopeEnter();
      HandleBlock(block_node);
      ScopeLeave(block_node);
    }
  }

  auto const endif_pc = uint16_t(function_->instructions.size());

  if (jf_pc != uint16_t(-1))
  {
    // The last condition (in an if statement that has no "else" part) jumps here
    function_->instructions[jf_pc].index = endif_pc;
    if (chain.kind == NodeKind::And)
    {
      FinaliseShortCircuitChain(chain, true, endif_pc);
    }
  }

  for (auto jump_pc : jump_pcs)
  {
    function_->instructions[jump_pc].index = endif_pc;
  }
}

void Generator::HandleUseStatement(IRNodePtr const &node)
{
  IRExpressionNodePtr state_name_node = ConvertToIRExpressionNodePtr(node->children[0]);
  IRNodePtr           list_node       = node->children[1];
  IRExpressionNodePtr alias_name_node = ConvertToIRExpressionNodePtr(node->children[2]);
  IRExpressionNodePtr n               = alias_name_node ? alias_name_node : state_name_node;
  HandleUseVariable(state_name_node->text, state_name_node->line, n);
}

void Generator::HandleUseAnyStatement(IRNodePtr const &node)
{
  for (auto const &c : node->children)
  {
    IRExpressionNodePtr child = ConvertToIRExpressionNodePtr(c);
    HandleUseVariable(child->text, child->line, child);
  }
}

void Generator::HandleUseVariable(std::string const &name, uint16_t line,
                                  IRExpressionNodePtr const &node)
{
  IRVariablePtr variable     = node->variable;
  IRFunctionPtr function     = node->function;
  uint16_t      type_id      = variable->type->id;
  auto const    scope_number = uint16_t(scopes_.size() - 1);
  variable->id               = function_->AddVariable(variable->name, type_id, scope_number);

  if (!variable->type->IsPrimitive())
  {
    Scope &scope = scopes_[scope_number];
    scope.objects.push_back(variable->id);
  }
  PushString(name, line);
  uint16_t                opcode = function->id;
  Executable::Instruction constructor_instruction(opcode);
  constructor_instruction.type_id     = type_id;
  constructor_instruction.data        = type_id;
  uint16_t constructor_instruction_pc = function_->AddInstruction(constructor_instruction);
  AddLineNumber(node->line, constructor_instruction_pc);
  Executable::Instruction declare_assign_instruction(Opcodes::VariableDeclareAssign);
  declare_assign_instruction.type_id     = type_id;
  declare_assign_instruction.index       = variable->id;
  declare_assign_instruction.data        = scope_number;
  uint16_t declare_assign_instruction_pc = function_->AddInstruction(declare_assign_instruction);
  AddLineNumber(node->line, declare_assign_instruction_pc);
}

void Generator::HandleContractStatement(IRNodePtr const &node)
{
  IRExpressionNodePtr contract_variable_node = ConvertToIRExpressionNodePtr(node->children[0]);
  IRExpressionNodePtr initialiser_node       = ConvertToIRExpressionNodePtr(node->children[2]);
  uint16_t            initialiser_type_id    = initialiser_node->type->id;
  IRVariablePtr       contract_variable      = contract_variable_node->variable;
  uint16_t            contract_id            = contract_variable->type->id;

  // The instance variable for a contract reference is just the contract identity provided by the
  // initialiser, so change the type of the contract variable to match the initialiser's type
  contract_variable->type = initialiser_node->type;

  auto const scope_number = uint16_t(scopes_.size() - 1);
  contract_variable->id =
      function_->AddVariable(contract_variable->name, initialiser_type_id, scope_number);

  if (!contract_variable->type->IsPrimitive())
  {
    Scope &scope = scopes_[scope_number];
    scope.objects.push_back(contract_variable->id);
  }

  HandleExpression(initialiser_node);
  Executable::Instruction instruction(Opcodes::ContractVariableDeclareAssign);
  instruction.type_id = contract_id;
  instruction.index   = contract_variable->id;
  instruction.data    = scope_number;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleVarStatement(IRNodePtr const &node)
{
  IRExpressionNodePtr variable_node = ConvertToIRExpressionNodePtr(node->children[0]);
  IRVariablePtr       variable      = variable_node->variable;
  uint16_t            type_id       = variable->type->id;

  auto const scope_number = uint16_t(scopes_.size() - 1);
  variable->id            = function_->AddVariable(variable->name, type_id, scope_number);

  if (!variable->type->IsPrimitive())
  {
    Scope &scope = scopes_[scope_number];
    scope.objects.push_back(variable->id);
  }

  if (node->node_kind == NodeKind::VarDeclarationStatement)
  {
    Executable::Instruction instruction(Opcodes::VariableDeclare);
    instruction.type_id = type_id;
    instruction.index   = variable->id;
    instruction.data    = scope_number;
    uint16_t pc         = function_->AddInstruction(instruction);
    AddLineNumber(node->line, pc);
  }
  else
  {
    IRExpressionNodePtr rhs;
    if (node->node_kind == NodeKind::VarDeclarationTypedAssignmentStatement)
    {
      rhs = ConvertToIRExpressionNodePtr(node->children[2]);
    }
    else
    {
      rhs = ConvertToIRExpressionNodePtr(node->children[1]);
    }

    HandleExpression(rhs);
    Executable::Instruction instruction(Opcodes::VariableDeclareAssign);
    instruction.type_id = type_id;
    instruction.index   = variable->id;
    instruction.data    = scope_number;
    uint16_t pc         = function_->AddInstruction(instruction);
    AddLineNumber(node->line, pc);
  }
}

void Generator::HandleReturnStatement(IRNodePtr const &node)
{
  if (node->children.empty())
  {
    Executable::Instruction instruction(Opcodes::Return);
    uint16_t                pc = function_->AddInstruction(instruction);
    AddLineNumber(node->line, pc);
    return;
  }

  IRExpressionNodePtr expression = ConvertToIRExpressionNodePtr(node->children[0]);
  HandleExpression(expression);

  Executable::Instruction instruction(Opcodes::ReturnValue);
  instruction.type_id = expression->type->id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleBreakStatement(IRNodePtr const &node)
{
  Loop &loop = loops_.back();

  Executable::Instruction instruction(Opcodes::Break);
  instruction.index       = 0;  // pc placeholder
  instruction.data        = loop.scope_number;
  uint16_t const break_pc = function_->AddInstruction(instruction);
  AddLineNumber(node->line, break_pc);
  loop.break_pcs.push_back(break_pc);
}

void Generator::HandleContinueStatement(IRNodePtr const &node)
{
  Loop &                  loop = loops_.back();
  Executable::Instruction instruction(Opcodes::Continue);
  instruction.index          = 0;  // pc placeholder
  instruction.data           = loop.scope_number;
  uint16_t const continue_pc = function_->AddInstruction(instruction);
  AddLineNumber(node->line, continue_pc);
  loop.continue_pcs.push_back(continue_pc);
}

void Generator::HandleAssignmentStatement(IRExpressionNodePtr const &node)
{
  IRExpressionNodePtr lhs                   = ConvertToIRExpressionNodePtr(node->children[0]);
  IRExpressionNodePtr rhs                   = ConvertToIRExpressionNodePtr(node->children[1]);
  bool const          assigning_to_variable = lhs->IsVariableExpression();
  if (assigning_to_variable)
  {
    HandleVariableAssignmentStatement(lhs, rhs);
  }
  else
  {
    // lhs->node_kind == NodeKind::Index
    HandleIndexedAssignmentStatement(node, lhs, rhs);
  }
}

void Generator::HandleInplaceAssignmentStatement(IRExpressionNodePtr const &node)
{
  IRExpressionNodePtr lhs                   = ConvertToIRExpressionNodePtr(node->children[0]);
  IRExpressionNodePtr rhs                   = ConvertToIRExpressionNodePtr(node->children[1]);
  bool const          assigning_to_variable = lhs->IsVariableExpression();
  if (assigning_to_variable)
  {
    HandleVariableInplaceAssignmentStatement(node, lhs, rhs);
  }
  else
  {
    // lhs->node_kind == NodeKind::Index
    HandleIndexedInplaceAssignmentStatement(node, lhs, rhs);
  }
}

void Generator::HandleVariableAssignmentStatement(IRExpressionNodePtr const &lhs,
                                                  IRExpressionNodePtr const &rhs)
{
  IRVariablePtr const &variable = lhs->variable;
  HandleExpression(rhs);
  Executable::Instruction instruction(Opcodes::PopToVariable);
  instruction.type_id = variable->type->id;
  instruction.index   = variable->id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(lhs->line, pc);
}

void Generator::HandleVariableInplaceAssignmentStatement(IRExpressionNodePtr const &node,
                                                         IRExpressionNodePtr const &lhs,
                                                         IRExpressionNodePtr const &rhs)
{
  IRVariablePtr const &variable         = lhs->variable;
  bool                 lhs_is_primitive = variable->type->IsPrimitive();
  TypeId               lhs_type_id      = variable->type->id;
  TypeId               rhs_type_id      = rhs->type->id;
  uint16_t             opcode           = Opcodes::Unknown;

  switch (node->node_kind)
  {
  case NodeKind::InplaceAdd:
  {
    opcode = GetInplaceArithmeticOpcode(
        lhs_is_primitive, lhs_type_id, rhs_type_id, Opcodes::VariablePrimitiveInplaceAdd,
        Opcodes::VariableObjectInplaceAdd, Opcodes::VariableObjectInplaceRightAdd);
    break;
  }
  case NodeKind::InplaceSubtract:
  {
    opcode = GetInplaceArithmeticOpcode(
        lhs_is_primitive, lhs_type_id, rhs_type_id, Opcodes::VariablePrimitiveInplaceSubtract,
        Opcodes::VariableObjectInplaceSubtract, Opcodes::VariableObjectInplaceRightSubtract);
    break;
  }
  case NodeKind::InplaceMultiply:
  {
    opcode = GetInplaceArithmeticOpcode(
        lhs_is_primitive, lhs_type_id, rhs_type_id, Opcodes::VariablePrimitiveInplaceMultiply,
        Opcodes::VariableObjectInplaceMultiply, Opcodes::VariableObjectInplaceRightMultiply);
    break;
  }
  case NodeKind::InplaceDivide:
  {
    opcode = GetInplaceArithmeticOpcode(
        lhs_is_primitive, lhs_type_id, rhs_type_id, Opcodes::VariablePrimitiveInplaceDivide,
        Opcodes::VariableObjectInplaceDivide, Opcodes::VariableObjectInplaceRightDivide);
    break;
  }
  case NodeKind::InplaceModulo:
  {
    opcode = Opcodes::VariablePrimitiveInplaceModulo;
    break;
  }
  default:
  {
    assert(false);
    break;
  }
  }  // switch

  HandleExpression(rhs);
  Executable::Instruction instruction(opcode);
  instruction.type_id = lhs_type_id;
  instruction.index   = variable->id;
  instruction.data    = rhs_type_id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(lhs->line, pc);
}

void Generator::HandleIndexedAssignmentStatement(IRExpressionNodePtr const &node,
                                                 IRExpressionNodePtr const &lhs,
                                                 IRExpressionNodePtr const &rhs)
{
  // Arrange for the container object to be pushed on to the stack
  IRExpressionNodePtr container_node = ConvertToIRExpressionNodePtr(lhs->children[0]);
  HandleExpression(container_node);
  // Arrange for the indices to be pushed on to the stack
  std::size_t const num_indices = lhs->children.size() - 1;
  for (std::size_t i = 1; i <= num_indices; ++i)
  {
    HandleExpression(ConvertToIRExpressionNodePtr(lhs->children[i]));
  }
  HandleExpression(rhs);
  uint16_t                container_type_id = container_node->type->id;
  uint16_t                type_id           = lhs->type->id;
  uint16_t                opcode            = node->function->id;
  Executable::Instruction instruction(opcode);
  instruction.type_id = type_id;
  instruction.data    = container_type_id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(lhs->line, pc);
}

void Generator::HandleIndexedInplaceAssignmentStatement(IRExpressionNodePtr const &node,
                                                        IRExpressionNodePtr const &lhs,
                                                        IRExpressionNodePtr const &rhs)
{
  // Arrange for the container object to be pushed on to the stack
  IRExpressionNodePtr container_node = ConvertToIRExpressionNodePtr(lhs->children[0]);
  HandleExpression(container_node);
  // Arrange for the indices to be pushed on to the stack
  std::size_t const num_indices = lhs->children.size() - 1;
  for (std::size_t i = 1; i <= num_indices; ++i)
  {
    HandleExpression(ConvertToIRExpressionNodePtr(lhs->children[i]));
  }

  uint16_t container_type_id = container_node->type->id;
  uint16_t type_id           = lhs->type->id;
  uint16_t rhs_type_id       = rhs->type->id;

  Executable::Instruction duplicate_instruction(Opcodes::Duplicate);
  duplicate_instruction.data = uint16_t(1 + num_indices);
  uint16_t duplicate_pc      = function_->AddInstruction(duplicate_instruction);
  AddLineNumber(lhs->line, duplicate_pc);

  uint16_t                get_indexed_value_opcode = lhs->function->id;
  Executable::Instruction get_indexed_value_instruction(get_indexed_value_opcode);
  get_indexed_value_instruction.type_id = type_id;
  get_indexed_value_instruction.data    = container_type_id;
  uint16_t get_indexed_value_pc         = function_->AddInstruction(get_indexed_value_instruction);
  AddLineNumber(lhs->line, get_indexed_value_pc);

  HandleExpression(rhs);

  bool lhs_is_primitive = lhs->type->IsPrimitive();

  uint16_t opcode = Opcodes::Unknown;
  switch (node->node_kind)
  {
  case NodeKind::InplaceAdd:
  {
    opcode =
        GetInplaceArithmeticOpcode(lhs_is_primitive, type_id, rhs_type_id, Opcodes::PrimitiveAdd,
                                   Opcodes::ObjectAdd, Opcodes::ObjectRightAdd);
    break;
  }
  case NodeKind::InplaceSubtract:
  {
    opcode = GetInplaceArithmeticOpcode(lhs_is_primitive, type_id, rhs_type_id,
                                        Opcodes::PrimitiveSubtract, Opcodes::ObjectSubtract,
                                        Opcodes::ObjectRightSubtract);
    break;
  }
  case NodeKind::InplaceMultiply:
  {
    opcode = GetInplaceArithmeticOpcode(lhs_is_primitive, type_id, rhs_type_id,
                                        Opcodes::PrimitiveMultiply, Opcodes::ObjectMultiply,
                                        Opcodes::ObjectRightMultiply);
    break;
  }
  case NodeKind::InplaceDivide:
  {
    opcode =
        GetInplaceArithmeticOpcode(lhs_is_primitive, type_id, rhs_type_id, Opcodes::PrimitiveDivide,
                                   Opcodes::ObjectDivide, Opcodes::ObjectRightDivide);
    break;
  }
  case NodeKind::InplaceModulo:
  {
    opcode = Opcodes::PrimitiveModulo;
    break;
  }
  default:
  {
    assert(false);
    break;
  }
  }  // switch

  Executable::Instruction instruction(opcode);
  instruction.type_id = type_id;
  instruction.data    = rhs_type_id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(lhs->line, pc);

  uint16_t                set_indexed_value_opcode = node->function->id;
  Executable::Instruction set_indexed_value_instruction(set_indexed_value_opcode);
  set_indexed_value_instruction.type_id = type_id;
  set_indexed_value_instruction.data    = container_type_id;
  uint16_t set_indexed_value_pc         = function_->AddInstruction(set_indexed_value_instruction);
  AddLineNumber(lhs->line, set_indexed_value_pc);
}

void Generator::HandleExpression(IRExpressionNodePtr const &node)
{
  switch (node->node_kind)
  {
  case NodeKind::Identifier:
  {
    HandleIdentifier(node);
    break;
  }
  case NodeKind::Integer8:
  {
    HandleInteger8(node);
    break;
  }
  case NodeKind::UnsignedInteger8:
  {
    HandleUnsignedInteger8(node);
    break;
  }
  case NodeKind::Integer16:
  {
    HandleInteger16(node);
    break;
  }
  case NodeKind::UnsignedInteger16:
  {
    HandleUnsignedInteger16(node);
    break;
  }
  case NodeKind::Integer32:
  {
    HandleInteger32(node);
    break;
  }
  case NodeKind::UnsignedInteger32:
  {
    HandleUnsignedInteger32(node);
    break;
  }
  case NodeKind::Integer64:
  {
    HandleInteger64(node);
    break;
  }
  case NodeKind::UnsignedInteger64:
  {
    HandleUnsignedInteger64(node);
    break;
  }
  case NodeKind::Float32:
  {
    HandleFloat32(node);
    break;
  }
  case NodeKind::Float64:
  {
    HandleFloat64(node);
    break;
  }
  case NodeKind::Fixed32:
  {
    HandleFixed32(node);
    break;
  }
  case NodeKind::Fixed64:
  {
    HandleFixed64(node);
    break;
  }
  case NodeKind::Fixed128:
  {
    HandleFixed128(node);
    break;
  }
  case NodeKind::String:
  {
    HandleString(node);
    break;
  }
  case NodeKind::True:
  {
    HandleTrue(node);
    break;
  }
  case NodeKind::False:
  {
    HandleFalse(node);
    break;
  }
  case NodeKind::InitialiserList:
  {
    HandleInitialiserList(node);
    break;
  }
  case NodeKind::Null:
  {
    HandleNull(node);
    break;
  }
  case NodeKind::PrefixInc:
  case NodeKind::PrefixDec:
  case NodeKind::PostfixInc:
  case NodeKind::PostfixDec:
  {
    HandlePrefixPostfixOp(node);
    break;
  }
  case NodeKind::Add:
  case NodeKind::Subtract:
  case NodeKind::Multiply:
  case NodeKind::Divide:
  case NodeKind::Modulo:
  case NodeKind::Equal:
  case NodeKind::NotEqual:
  case NodeKind::LessThan:
  case NodeKind::LessThanOrEqual:
  case NodeKind::GreaterThan:
  case NodeKind::GreaterThanOrEqual:
  {
    HandleBinaryOp(node);
    break;
  }
  case NodeKind::And:
  case NodeKind::Or:
  {
    HandleShortCircuitOp(nullptr, node);
    break;
  }
  case NodeKind::Negate:
  case NodeKind::Not:
  {
    HandleUnaryOp(node);
    break;
  }
  case NodeKind::Index:
  {
    HandleIndexOp(node);
    break;
  }
  case NodeKind::Dot:
  {
    HandleDotOp(node);
    break;
  }
  case NodeKind::Invoke:
  {
    HandleInvokeOp(node);
    break;
  }
  default:
  {
    assert(false);
    break;
  }
  }  // switch
}

void Generator::HandleIdentifier(IRExpressionNodePtr const &node)
{
  IRVariablePtr           variable = node->variable;
  Executable::Instruction instruction(Opcodes::PushVariable);
  instruction.type_id = variable->type->id;
  instruction.index   = variable->id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleInteger8(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto                    value = static_cast<int8_t>(std::atoi(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::Int8));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleUnsignedInteger8(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto                    value = static_cast<uint8_t>(std::atoi(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::UInt8));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleInteger16(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto                    value = static_cast<int16_t>(std::atoi(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::Int16));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleUnsignedInteger16(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto                    value = static_cast<uint16_t>(std::atoi(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::UInt16));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleInteger32(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto                    value = static_cast<int32_t>(std::atoi(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::Int32));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleUnsignedInteger32(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto                    value = static_cast<uint32_t>(std::atoll(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::UInt32));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleInteger64(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto                    value = static_cast<int64_t>(std::atoll(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::Int64));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleUnsignedInteger64(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto value        = static_cast<uint64_t>(std::strtoull(node->text.c_str(), nullptr, 10));
  instruction.index = AddConstant(Variant(value, TypeIds::UInt64));
  uint16_t pc       = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleFloat32(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  auto                    value = float(std::atof(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::Float32));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleFloat64(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  double                  value = std::atof(node->text.c_str());
  instruction.index             = AddConstant(Variant(value, TypeIds::Float64));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleFixed32(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  fixed_point::fp32_t     value = fixed_point::fp32_t(std::atof(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::Fixed32));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleFixed64(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushConstant);
  fixed_point::fp64_t     value = fixed_point::fp64_t(std::atof(node->text.c_str()));
  instruction.index             = AddConstant(Variant(value, TypeIds::Fixed64));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleFixed128(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushLargeConstant);
  fixed_point::fp128_t    value = fixed_point::fp128_t(std::atof(node->text.c_str()));
  instruction.index             = AddLargeConstant(Executable::LargeConstant(value));
  uint16_t pc                   = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleString(IRExpressionNodePtr const &node)
{
  PushString(node->text.substr(1, node->text.size() - 2), node->line);
}

void Generator::PushString(std::string const &s, uint16_t line)
{
  uint16_t index;
  auto     it = strings_map_.find(s);
  if (it != strings_map_.end())
  {
    index = it->second;
  }
  else
  {
    index = uint16_t(executable_.strings.size());
    executable_.strings.push_back(s);
    strings_map_[s] = index;
  }
  Executable::Instruction instruction(Opcodes::PushString);
  instruction.index = index;
  uint16_t pc       = function_->AddInstruction(instruction);
  AddLineNumber(line, pc);
}

void Generator::HandleTrue(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushTrue);
  uint16_t                pc = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleFalse(IRExpressionNodePtr const &node)
{
  Executable::Instruction instruction(Opcodes::PushFalse);
  uint16_t                pc = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleInitialiserList(IRExpressionNodePtr const &node)
{
  auto const &node_type{node->type};
  if (!node_type->IsInstantiation() || node_type->template_type->name != "Array" ||
      node_type->template_parameter_types.empty())
  // NB: if there's no better way to check this, there _should_ be a better way to check this
  {
    return;  // hypothetical no-op, in fact this situation is catched earlier
  }
  assert(node->children.size() <= static_cast<std::size_t>(std::numeric_limits<uint16_t>::max()) &&
         !node_type->template_parameter_types.empty() &&
         bool(node_type->template_parameter_types.front()));

  for (auto const &expr : node->children)
  {
    HandleExpression(ConvertToIRExpressionNodePtr(expr));
  }

  Executable::Instruction instruction{Opcodes::InitialiseArray};
  instruction.type_id = node_type->id;
  instruction.data    = static_cast<uint16_t>(node->children.size());
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleNull(IRExpressionNodePtr const &node)
{
  assert(!node->type->IsPrimitive());
  Executable::Instruction instruction(Opcodes::PushNull);
  instruction.type_id = node->type->id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandlePrefixPostfixOp(IRExpressionNodePtr const &node)
{
  IRExpressionNodePtr operand               = ConvertToIRExpressionNodePtr(node->children[0]);
  bool const          assigning_to_variable = operand->IsVariableExpression();
  if (assigning_to_variable)
  {
    HandleVariablePrefixPostfixOp(node, operand);
  }
  else
  {
    // operand->node_kind == NodeKind::Index
    HandleIndexedPrefixPostfixOp(node, operand);
  }
}

void Generator::HandleBinaryOp(IRExpressionNodePtr const &node)
{
  IRExpressionNodePtr lhs = ConvertToIRExpressionNodePtr(node->children[0]);
  IRExpressionNodePtr rhs = ConvertToIRExpressionNodePtr(node->children[1]);
  HandleExpression(lhs);
  HandleExpression(rhs);

  bool     lhs_is_primitive = lhs->type->IsPrimitive();
  TypeId   node_type_id     = node->type->id;
  TypeId   lhs_type_id      = lhs->type->id;
  TypeId   rhs_type_id      = rhs->type->id;
  TypeId   type_id          = lhs_type_id;
  TypeId   other_type_id    = lhs_type_id;
  uint16_t opcode           = Opcodes::Unknown;

  switch (node->node_kind)
  {
  case NodeKind::Add:
  {
    opcode = GetArithmeticOpcode(lhs_is_primitive, node_type_id, lhs_type_id, rhs_type_id,
                                 Opcodes::PrimitiveAdd, Opcodes::ObjectAdd, Opcodes::ObjectLeftAdd,
                                 Opcodes::ObjectRightAdd, type_id, other_type_id);

    break;
  }
  case NodeKind::Subtract:
  {
    opcode = GetArithmeticOpcode(lhs_is_primitive, node_type_id, lhs_type_id, rhs_type_id,
                                 Opcodes::PrimitiveSubtract, Opcodes::ObjectSubtract,
                                 Opcodes::ObjectLeftSubtract, Opcodes::ObjectRightSubtract, type_id,
                                 other_type_id);
    break;
  }
  case NodeKind::Multiply:
  {
    opcode = GetArithmeticOpcode(lhs_is_primitive, node_type_id, lhs_type_id, rhs_type_id,
                                 Opcodes::PrimitiveMultiply, Opcodes::ObjectMultiply,
                                 Opcodes::ObjectLeftMultiply, Opcodes::ObjectRightMultiply, type_id,
                                 other_type_id);
    break;
  }
  case NodeKind::Divide:
  {
    opcode = GetArithmeticOpcode(lhs_is_primitive, node_type_id, lhs_type_id, rhs_type_id,
                                 Opcodes::PrimitiveDivide, Opcodes::ObjectDivide,
                                 Opcodes::ObjectLeftDivide, Opcodes::ObjectRightDivide, type_id,
                                 other_type_id);
    break;
  }
  case NodeKind::Modulo:
  {
    opcode = Opcodes::PrimitiveModulo;
    break;
  }
  case NodeKind::Equal:
  {
    opcode = lhs_is_primitive ? Opcodes::PrimitiveEqual : Opcodes::ObjectEqual;
    break;
  }
  case NodeKind::NotEqual:
  {
    opcode = lhs_is_primitive ? Opcodes::PrimitiveNotEqual : Opcodes::ObjectNotEqual;
    break;
  }
  case NodeKind::LessThan:
  {
    opcode = lhs_is_primitive ? Opcodes::PrimitiveLessThan : Opcodes::ObjectLessThan;
    break;
  }
  case NodeKind::LessThanOrEqual:
  {
    opcode = lhs_is_primitive ? Opcodes::PrimitiveLessThanOrEqual : Opcodes::ObjectLessThanOrEqual;
    break;
  }
  case NodeKind::GreaterThan:
  {
    opcode = lhs_is_primitive ? Opcodes::PrimitiveGreaterThan : Opcodes::ObjectGreaterThan;
    break;
  }
  case NodeKind::GreaterThanOrEqual:
  {
    opcode =
        lhs_is_primitive ? Opcodes::PrimitiveGreaterThanOrEqual : Opcodes::ObjectGreaterThanOrEqual;
    break;
  }
  default:
  {
    assert(false);
    break;
  }
  }  // switch

  Executable::Instruction instruction(opcode);
  instruction.type_id = type_id;
  instruction.data    = other_type_id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleUnaryOp(IRExpressionNodePtr const &node)
{
  IRExpressionNodePtr operand      = ConvertToIRExpressionNodePtr(node->children[0]);
  uint16_t            type_id      = TypeIds::Unknown;
  bool                is_primitive = operand->type->IsPrimitive();
  uint16_t            opcode       = Opcodes::Unknown;
  switch (node->node_kind)
  {
  case NodeKind::Negate:
  {
    opcode  = is_primitive ? Opcodes::PrimitiveNegate : Opcodes::ObjectNegate;
    type_id = node->type->id;
    break;
  }
  case NodeKind::Not:
  {
    opcode  = Opcodes::Not;
    type_id = node->type->id;
    break;
  }
  default:
  {
    assert(false);
    break;
  }
  }  // switch
  HandleExpression(operand);
  Executable::Instruction instruction(opcode);
  instruction.type_id = type_id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

Generator::Chain Generator::HandleConditionExpression(IRBlockNodePtr const &     block_node,
                                                      IRExpressionNodePtr const &node)
{
  if ((node->node_kind == NodeKind::And) || (node->node_kind == NodeKind::Or))
  {
    return HandleShortCircuitOp(block_node, node);
  }

  HandleExpression(node);
  return Chain();
}

Generator::Chain Generator::HandleShortCircuitOp(IRNodePtr const &          parent_node,
                                                 IRExpressionNodePtr const &node)
{
  IRExpressionNodePtr lhs = ConvertToIRExpressionNodePtr(node->children[0]);
  IRExpressionNodePtr rhs = ConvertToIRExpressionNodePtr(node->children[1]);

  NodeKind parent_node_kind = NodeKind::Unknown;
  if (parent_node)
  {
    parent_node_kind = parent_node->node_kind;
  }

  bool const is_condition_chain = (parent_node_kind == NodeKind::WhileStatement) ||
                                  (parent_node_kind == NodeKind::If) ||
                                  (parent_node_kind == NodeKind::ElseIf);
  bool const in_chain = node->node_kind == parent_node_kind;

  Chain lhs_chain;
  if ((lhs->node_kind == NodeKind::And) || (lhs->node_kind == NodeKind::Or))
  {
    lhs_chain = HandleShortCircuitOp(node, lhs);
  }
  else
  {
    HandleExpression(lhs);
  }

  Executable::Instruction jump_instruction(Opcodes::Unknown);  // opcode placeholder
  jump_instruction.index = 0;                                  // pc placeholder
  uint16_t const jump_pc = function_->AddInstruction(jump_instruction);
  AddLineNumber(node->line, jump_pc);

  Chain rhs_chain;
  if ((rhs->node_kind == NodeKind::And) || (rhs->node_kind == NodeKind::Or))
  {
    rhs_chain = HandleShortCircuitOp(node, rhs);
  }
  else
  {
    HandleExpression(rhs);
  }

  Chain chain(node->node_kind);
  chain.Append(jump_pc);
  chain.Append(lhs_chain.pcs);
  chain.Append(rhs_chain.pcs);

  if (is_condition_chain || in_chain)
  {
    return chain;
  }

  auto const destination_pc = uint16_t(function_->instructions.size());
  FinaliseShortCircuitChain(chain, false, destination_pc);
  return Chain();
}

void Generator::FinaliseShortCircuitChain(Chain const &chain, bool is_condition_chain,
                                          uint16_t destination_pc)
{
  uint16_t opcode;
  if (is_condition_chain)
  {
    opcode = chain.kind == NodeKind::And ? Opcodes::JumpIfFalse : Opcodes::JumpIfTrue;
  }
  else
  {
    opcode = chain.kind == NodeKind::And ? Opcodes::JumpIfFalseOrPop : Opcodes::JumpIfTrueOrPop;
  }

  for (auto pc : chain.pcs)
  {
    Executable::Instruction &instruction = function_->instructions[pc];
    instruction.opcode                   = opcode;
    instruction.index                    = destination_pc;
  }
}

void Generator::HandleIndexOp(IRExpressionNodePtr const &node)
{
  // Arrange for the container object to be pushed on to the stack
  IRExpressionNodePtr container_node = ConvertToIRExpressionNodePtr(node->children[0]);
  HandleExpression(container_node);
  // Arrange for the indices to be pushed on to the stack
  std::size_t const num_indices = node->children.size() - 1;
  for (std::size_t i = 1; i <= num_indices; ++i)
  {
    HandleExpression(ConvertToIRExpressionNodePtr(node->children[i]));
  }
  uint16_t                container_type_id        = container_node->type->id;
  uint16_t                type_id                  = node->type->id;
  uint16_t                get_indexed_value_opcode = node->function->id;
  Executable::Instruction instruction(get_indexed_value_opcode);
  instruction.type_id = type_id;
  instruction.data    = container_type_id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(node->line, pc);
}

void Generator::HandleDotOp(IRExpressionNodePtr const &node)
{
  IRExpressionNodePtr lhs = ConvertToIRExpressionNodePtr(node->children[0]);
  if ((lhs->IsVariableExpression()) || (lhs->IsLVExpression()) || (lhs->IsRVExpression()))
  {
    // Arrange for the invoker to be pushed on to the stack
    HandleExpression(ConvertToIRExpressionNodePtr(lhs));
  }
}

void Generator::HandleInvokeOp(IRExpressionNodePtr const &node)
{
  IRExpressionNodePtr lhs            = ConvertToIRExpressionNodePtr(node->children[0]);
  IRFunctionPtr       function       = node->function;
  uint16_t            return_type_id = node->type->id;

  if (lhs->function_invoker_is_instance)
  {
    // Arrange for the invoker to be pushed on to the stack
    HandleExpression(ConvertToIRExpressionNodePtr(lhs));
  }
  // Arrange for the function parameters to be pushed on to the stack
  for (std::size_t i = 1; i < node->children.size(); ++i)
  {
    HandleExpression(ConvertToIRExpressionNodePtr(node->children[i]));
  }
  if (function->function_kind == FunctionKind::UserDefinedFreeFunction)
  {
    Executable::Instruction instruction(Opcodes::InvokeUserDefinedFreeFunction);
    instruction.index   = function->id;
    instruction.type_id = return_type_id;
    uint16_t pc         = function_->AddInstruction(instruction);
    AddLineNumber(node->line, pc);
  }
  else if (function->function_kind == FunctionKind::UserDefinedContractFunction)
  {
    uint16_t                function_id = function->id;
    uint16_t                contract_id = lhs->type->id;
    Executable::Instruction instruction(Opcodes::InvokeContractFunction);
    instruction.index   = function_id;
    instruction.type_id = return_type_id;
    instruction.data    = contract_id;
    uint16_t pc         = function_->AddInstruction(instruction);
    AddLineNumber(node->line, pc);
  }
  else
  {
    // Opcode-invoked free function
    // Opcode-invoked constructor
    // Opcode-invoked static member function
    // Opcode-invoked member function
    uint16_t                opcode = function->id;
    Executable::Instruction instruction(opcode);
    instruction.type_id = return_type_id;
    if (lhs->type)
    {
      // Store the invoker's type id
      instruction.data = lhs->type->id;
    }
    uint16_t pc = function_->AddInstruction(instruction);
    AddLineNumber(node->line, pc);
  }
}

void Generator::HandleVariablePrefixPostfixOp(IRExpressionNodePtr const &node,
                                              IRExpressionNodePtr const &operand)
{
  uint16_t opcode = Opcodes::Unknown;
  switch (node->node_kind)
  {
  case NodeKind::PrefixInc:
  {
    opcode = Opcodes::VariablePrefixInc;
    break;
  }
  case NodeKind::PrefixDec:
  {
    opcode = Opcodes::VariablePrefixDec;
    break;
  }
  case NodeKind::PostfixInc:
  {
    opcode = Opcodes::VariablePostfixInc;
    break;
  }
  case NodeKind::PostfixDec:
  {
    opcode = Opcodes::VariablePostfixDec;
    break;
  }
  default:
  {
    assert(false);
    break;
  }
  }  // switch
  IRVariablePtr const &   variable = operand->variable;
  Executable::Instruction instruction(opcode);
  instruction.type_id = variable->type->id;
  instruction.index   = variable->id;
  uint16_t pc         = function_->AddInstruction(instruction);
  AddLineNumber(operand->line, pc);
}

void Generator::HandleIndexedPrefixPostfixOp(IRExpressionNodePtr const &node,
                                             IRExpressionNodePtr const &operand)
{
  // Arrange for the container object to be pushed on to the stack
  IRExpressionNodePtr container_node = ConvertToIRExpressionNodePtr(operand->children[0]);
  HandleExpression(container_node);
  // Arrange for the indices to be pushed on to the stack
  std::size_t const num_indices = operand->children.size() - 1;
  for (std::size_t i = 1; i <= num_indices; ++i)
  {
    HandleExpression(ConvertToIRExpressionNodePtr(operand->children[i]));
  }
  uint16_t                container_type_id = container_node->type->id;
  uint16_t                type_id           = operand->type->id;
  Executable::Instruction duplicate_instruction(Opcodes::Duplicate);
  duplicate_instruction.data = uint16_t(1 + num_indices);
  uint16_t duplicate_pc      = function_->AddInstruction(duplicate_instruction);
  AddLineNumber(operand->line, duplicate_pc);
  uint16_t                get_indexed_value_opcode = operand->function->id;
  Executable::Instruction get_indexed_value_instruction(get_indexed_value_opcode);
  get_indexed_value_instruction.type_id = type_id;
  get_indexed_value_instruction.data    = container_type_id;
  uint16_t get_indexed_value_pc         = function_->AddInstruction(get_indexed_value_instruction);
  AddLineNumber(operand->line, get_indexed_value_pc);

  bool     prefix = true;
  uint16_t opcode = Opcodes::Unknown;
  switch (node->node_kind)
  {
  case NodeKind::PrefixInc:
  {
    opcode = Opcodes::Inc;
    break;
  }
  case NodeKind::PrefixDec:
  {
    opcode = Opcodes::Dec;
    break;
  }
  case NodeKind::PostfixInc:
  {
    opcode = Opcodes::Inc;
    prefix = false;
    break;
  }
  case NodeKind::PostfixDec:
  {
    opcode = Opcodes::Dec;
    prefix = false;
    break;
  }
  default:
  {
    assert(false);
    break;
  }
  }  // switch

  Executable::Instruction instruction(opcode);
  instruction.type_id = type_id;

  Executable::Instruction duplicate_insert_instruction(Opcodes::DuplicateInsert);
  duplicate_insert_instruction.data = uint16_t(1 + num_indices);

  if (prefix)
  {
    uint16_t pc = function_->AddInstruction(instruction);
    AddLineNumber(operand->line, pc);
    uint16_t duplicate_insert_pc = function_->AddInstruction(duplicate_insert_instruction);
    AddLineNumber(operand->line, duplicate_insert_pc);
  }
  else
  {
    uint16_t duplicate_insert_pc = function_->AddInstruction(duplicate_insert_instruction);
    AddLineNumber(operand->line, duplicate_insert_pc);
    uint16_t pc = function_->AddInstruction(instruction);
    AddLineNumber(operand->line, pc);
  }

  uint16_t                set_indexed_value_opcode = node->function->id;
  Executable::Instruction set_indexed_value_instruction(set_indexed_value_opcode);
  set_indexed_value_instruction.type_id = type_id;
  set_indexed_value_instruction.data    = container_type_id;
  uint16_t set_indexed_value_instruction_pc =
      function_->AddInstruction(set_indexed_value_instruction);
  AddLineNumber(operand->line, set_indexed_value_instruction_pc);
}

void Generator::ScopeEnter()
{
  scopes_.emplace_back();
}

void Generator::ScopeLeave(IRBlockNodePtr const &block_node)
{
  auto const scope_number = uint16_t(scopes_.size() - 1);
  Scope &    scope        = scopes_[scope_number];
  if (!scope.objects.empty())
  {
    // Note: the function definition block does not require a Destruct instruction
    // because the Return or ReturnValue instructions, which must always be
    // present, already take care of destructing objects
    if (block_node->node_kind != NodeKind::FunctionDefinition)
    {
      // Arrange for all live objects with scope >= scope_number to be destructed
      Executable::Instruction instruction(Opcodes::Destruct);
      instruction.data = scope_number;
      uint16_t pc      = function_->AddInstruction(instruction);
      AddLineNumber(block_node->block_terminator_line, pc);
    }
  }
  scopes_.pop_back();
}

uint16_t Generator::AddConstant(Variant const &c)
{
  uint16_t index;
  auto     it = constants_map_.find(c);
  if (it != constants_map_.end())
  {
    index = it->second;
  }
  else
  {
    index = uint16_t(executable_.constants.size());
    executable_.constants.push_back(c);
    constants_map_[c] = index;
  }
  return index;
}

uint16_t Generator::AddLargeConstant(Executable::LargeConstant const &c)
{
  uint16_t index;
  auto     it = large_constants_map_.find(c);
  if (it != large_constants_map_.end())
  {
    index = it->second;
  }
  else
  {
    index = uint16_t(executable_.large_constants.size());
    executable_.large_constants.push_back(c);
    large_constants_map_[c] = index;
  }
  return index;
}

uint16_t Generator::GetInplaceArithmeticOpcode(bool is_primitive, TypeId lhs_type_id,
                                               TypeId rhs_type_id, uint16_t opcode1,
                                               uint16_t opcode2, uint16_t opcode3)
{
  uint16_t opcode;
  if (is_primitive)
  {
    opcode = opcode1;
  }
  else if (lhs_type_id == rhs_type_id)
  {
    opcode = opcode2;
  }
  else
  {
    opcode = opcode3;
  }
  return opcode;
}

uint16_t Generator::GetArithmeticOpcode(bool lhs_is_primitive, TypeId node_type_id,
                                        TypeId lhs_type_id, TypeId rhs_type_id, uint16_t opcode1,
                                        uint16_t opcode2, uint16_t opcode3, uint16_t opcode4,
                                        TypeId &type_id, TypeId &other_type_id)
{
  uint16_t opcode;
  if (lhs_type_id != node_type_id)
  {
    // left op
    opcode        = opcode3;
    type_id       = lhs_type_id;
    other_type_id = rhs_type_id;
  }
  else if (rhs_type_id != node_type_id)
  {
    // right op
    opcode        = opcode4;
    type_id       = rhs_type_id;
    other_type_id = lhs_type_id;
  }
  else
  {
    if (lhs_is_primitive)
    {
      opcode        = opcode1;
      type_id       = lhs_type_id;
      other_type_id = lhs_type_id;
    }
    else
    {
      opcode        = opcode2;
      type_id       = lhs_type_id;
      other_type_id = lhs_type_id;
    }
  }
  return opcode;
}

bool Generator::ConstantComparator::operator()(Variant const &lhs, Variant const &rhs) const
{
  if (lhs.type_id < rhs.type_id)
  {
    return true;
  }
  if (lhs.type_id > rhs.type_id)
  {
    return false;
  }
  switch (lhs.type_id)
  {
  case TypeIds::Int8:
  {
    return lhs.primitive.i8 < rhs.primitive.i8;
  }
  case TypeIds::UInt8:
  {
    return lhs.primitive.ui8 < rhs.primitive.ui8;
  }
  case TypeIds::Int16:
  {
    return lhs.primitive.i16 < rhs.primitive.i16;
  }
  case TypeIds::UInt16:
  {
    return lhs.primitive.ui16 < rhs.primitive.ui16;
  }
  case TypeIds::Int32:
  {
    return lhs.primitive.i32 < rhs.primitive.i32;
  }
  case TypeIds::UInt32:
  {
    return lhs.primitive.ui32 < rhs.primitive.ui32;
  }
  case TypeIds::Int64:
  {
    return lhs.primitive.i64 < rhs.primitive.i64;
  }
  case TypeIds::UInt64:
  {
    return lhs.primitive.ui64 < rhs.primitive.ui64;
  }
  case TypeIds::Float32:
  {
    return lhs.primitive.f32 < rhs.primitive.f32;
  }
  case TypeIds::Float64:
  {
    return lhs.primitive.f64 < rhs.primitive.f64;
  }
  case TypeIds::Fixed32:
  {
    return fixed_point::fp32_t::FromBase(lhs.primitive.i32) <
           fixed_point::fp32_t::FromBase(rhs.primitive.i32);
  }
  case TypeIds::Fixed64:
  {
    return fixed_point::fp64_t::FromBase(lhs.primitive.i64) <
           fixed_point::fp64_t::FromBase(rhs.primitive.i64);
  }
  default:
  {
    assert(false);
    return false;
  }
  }  // switch
}

bool Generator::LargeConstantComparator::operator()(Executable::LargeConstant const &lhs,
                                                    Executable::LargeConstant const &rhs) const
{
  if (lhs.type_id < rhs.type_id)
  {
    return true;
  }
  if (lhs.type_id > rhs.type_id)
  {
    return false;
  }
  switch (lhs.type_id)
  {
  case TypeIds::Fixed128:
  {
    return lhs.fp128 < rhs.fp128;
  }
  default:
  {
    assert(false);
    return false;
  }
  }  // switch
}

}  // namespace vm
}  // namespace fetch
