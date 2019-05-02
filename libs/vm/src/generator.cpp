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
#include <sstream>

namespace fetch {
namespace vm {

void Generator::Generate(BlockNodePtr const &root, TypeInfoTable const &type_info_table,
                         std::string const &name, Script &script)
{
  script_ = Script(name, type_info_table);
  scopes_.clear();
  loops_.clear();
  strings_map_.clear();
  strings_.clear();
  function_ = nullptr;
  CreateFunctions(root);
  HandleBlock(root);
  script_.strings = std::move(strings_);
  script          = std::move(script_);
  function_       = nullptr;
  strings_map_.clear();
  loops_.clear();
  scopes_.clear();
}

void Generator::CreateFunctions(BlockNodePtr const &root)
{
  for (NodePtr const &child : root->block_children)
  {
    switch (child->kind)
    {
    case Node::Kind::FunctionDefinitionStatement:
    {
      BlockNodePtr        function_definition_node = ConvertToBlockNodePtr(child);
      NodePtr             annotations_node         = function_definition_node->children[0];
      Script::Annotations annotations;
      CreateAnnotations(annotations_node, annotations);
      ExpressionNodePtr identifier_node =
          ConvertToExpressionNodePtr(function_definition_node->children[1]);
      FunctionPtr        f              = identifier_node->function;
      std::string const &name           = f->name;
      int const          num_parameters = (int)f->parameter_variables.size();
      TypeId const       return_type_id = f->return_type->id;
      Script::Function   function(name, annotations, num_parameters, return_type_id);
      for (VariablePtr const &v : f->parameter_variables)
      {
        v->index = function.AddVariable(v->name, v->type->id);
      }
      f->index = script_.AddFunction(function);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }
}

void Generator::CreateAnnotations(NodePtr const &annotations_node, Script::Annotations &annotations)
{
  annotations.clear();
  if (annotations_node == nullptr)
  {
    return;
  }
  for (NodePtr const &annotation_node : annotations_node->children)
  {
    Script::Annotation annotation;
    annotation.name = annotation_node->token.text;
    for (NodePtr const &annotation_element_node : annotation_node->children)
    {
      Script::AnnotationElement element;
      if (annotation_element_node->kind == Node::Kind::AnnotationNameValuePair)
      {
        element.type = Script::AnnotationElementType::NameValuePair;
        SetAnnotationLiteral(annotation_element_node->children[0], element.name);
        SetAnnotationLiteral(annotation_element_node->children[1], element.value);
      }
      else
      {
        element.type = Script::AnnotationElementType::Value;
        SetAnnotationLiteral(annotation_element_node, element.value);
      }
      annotation.elements.push_back(element);
    }
    annotations.push_back(annotation);
  }
}

void Generator::SetAnnotationLiteral(NodePtr const &node, Script::AnnotationLiteral &literal)
{
  const std::string &text = node->token.text;
  switch (node->kind)
  {
  case Node::Kind::True:
  {
    literal.SetBoolean(true);
    break;
  }
  case Node::Kind::False:
  {
    literal.SetBoolean(false);
    break;
  }
  case Node::Kind::Integer64:
  {
    int64_t i = atol(text.c_str());
    literal.SetInteger(i);
    break;
  }
  case Node::Kind::Float64:
  {
    double r = atof(text.c_str());
    literal.SetReal(r);
    break;
  }
  case Node::Kind::String:
  {
    std::string s = text.substr(1, text.size() - 2);
    literal.SetString(s);
    break;
  }
  case Node::Kind::Identifier:
  {
    literal.SetIdentifier(text);
    break;
  }
  default:
  {
    break;
  }
  }  // switch
}

void Generator::HandleBlock(BlockNodePtr const &block)
{
  for (NodePtr const &child : block->block_children)
  {
    switch (child->kind)
    {
    case Node::Kind::FunctionDefinitionStatement:
    {
      HandleFunctionDefinitionStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::WhileStatement:
    {
      HandleWhileStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::ForStatement:
    {
      HandleForStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::IfStatement:
    {
      HandleIfStatement(child);
      break;
    }
    case Node::Kind::VarDeclarationStatement:
    case Node::Kind::VarDeclarationTypedAssignmentStatement:
    case Node::Kind::VarDeclarationTypelessAssignmentStatement:
    {
      HandleVarStatement(child);
      break;
    }
    case Node::Kind::ReturnStatement:
    {
      HandleReturnStatement(child);
      break;
    }
    case Node::Kind::BreakStatement:
    {
      HandleBreakStatement(child);
      break;
    }
    case Node::Kind::ContinueStatement:
    {
      HandleContinueStatement(child);
      break;
    }
    case Node::Kind::AssignOp:
    case Node::Kind::ModuloAssignOp:
    case Node::Kind::AddAssignOp:
    case Node::Kind::SubtractAssignOp:
    case Node::Kind::MultiplyAssignOp:
    case Node::Kind::DivideAssignOp:
    {
      HandleAssignmentStatement(ConvertToExpressionNodePtr(child));
      break;
    }
    default:
    {
      ExpressionNodePtr expression = ConvertToExpressionNodePtr(child);
      HandleExpression(expression);
      if (expression->type->id != TypeIds::Void)
      {
        // The result of the expression is not consumed, so issue an
        // instruction that will pop it and throw it away
        Script::Instruction instruction(Opcodes::Discard, expression->token.line);
        instruction.type_id = expression->type->id;
        function_->AddInstruction(instruction);
      }
      break;
    }
    }  // switch
  }
}

void Generator::HandleFunctionDefinitionStatement(BlockNodePtr const &node)
{
  ExpressionNodePtr identifier_node = ConvertToExpressionNodePtr(node->children[1]);
  FunctionPtr       f               = identifier_node->function;
  function_                         = &(script_.functions[f->index]);

  ScopeEnter();
  HandleBlock(node);
  ScopeLeave(node);

  if (f->return_type->id == TypeIds::Void)
  {
    Script::Instruction instruction(Opcodes::Return, node->block_terminator.line);
    instruction.type_id  = TypeIds::Void;
    instruction.data.i32 = 0;  // scope number
    function_->AddInstruction(instruction);
  }

  function_ = nullptr;
}

void Generator::HandleWhileStatement(BlockNodePtr const &node)
{
  Index const       continue_pc = (Index)function_->instructions.size();
  ExpressionNodePtr condition   = ConvertToExpressionNodePtr(node->children[0]);

  HandleExpression(condition);

  Index const         jf_pc = (Index)function_->instructions.size();
  Script::Instruction jf_instruction(Opcodes::JumpIfFalse, node->token.line);
  jf_instruction.index = 0;  // pc placeholder

  function_->AddInstruction(jf_instruction);
  ScopeEnter();

  int const scope_number = static_cast<int>(scopes_.size()) - 1;

  loops_.push_back(Loop());
  loops_.back().scope_number = scope_number;

  HandleBlock(node);
  ScopeLeave(node);

  Script::Instruction jump_instruction(Opcodes::Jump, node->block_terminator.line);

  jump_instruction.index = continue_pc;
  function_->AddInstruction(jump_instruction);

  Index const endwhile_pc              = (Index)function_->instructions.size();
  function_->instructions[jf_pc].index = endwhile_pc;
  Loop &loop                           = loops_.back();

  for (auto const &jump_pc : loop.continue_pcs)
  {
    function_->instructions[jump_pc].index = continue_pc;
  }

  for (auto const &jump_pc : loop.break_pcs)
  {
    function_->instructions[jump_pc].index = endwhile_pc;
  }

  loops_.pop_back();
}

void Generator::HandleForStatement(BlockNodePtr const &node)
{
  ExpressionNodePtr identifier_node = ConvertToExpressionNodePtr(node->children[0]);
  VariablePtr       v               = identifier_node->variable;
  int const         arity           = static_cast<int>(node->children.size()) - 1;

  for (int i = 1; i <= arity; ++i)
  {
    HandleExpression(ConvertToExpressionNodePtr(node->children[std::size_t(i)]));
  }

  // Add the for-loop variable
  v->index = function_->AddVariable(v->name, v->type->id);

  Script::Instruction init_instruction(Opcodes::ForRangeInit, node->token.line);
  init_instruction.index    = v->index;  // frame-relative index of the for-loop variable
  init_instruction.type_id  = v->type->id;
  init_instruction.data.i32 = arity;  // 2 or 3 range elements

  function_->AddInstruction(init_instruction);
  Script::Instruction iterate_instruction(Opcodes::ForRangeIterate, node->token.line);
  iterate_instruction.index    = 0;      // pc placeholder
  iterate_instruction.data.i32 = arity;  // 2 or 3 range elements

  Index const iterate_pc = function_->AddInstruction(iterate_instruction);

  ScopeEnter();

  int const scope_number = static_cast<int>(scopes_.size()) - 1;

  loops_.push_back(Loop());
  loops_.back().scope_number = scope_number;

  HandleBlock(node);
  ScopeLeave(node);

  Script::Instruction jump_instruction(Opcodes::Jump, node->block_terminator.line);
  jump_instruction.index = iterate_pc;
  function_->AddInstruction(jump_instruction);

  Script::Instruction terminate_instruction(Opcodes::ForRangeTerminate,
                                            node->block_terminator.line);

  terminate_instruction.index   = v->index;
  terminate_instruction.type_id = v->type->id;
  Index const terminate_pc      = function_->AddInstruction(terminate_instruction);
  Loop &      loop              = loops_.back();

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

void Generator::HandleIfStatement(NodePtr const &node)
{
  Index              jf_pc = Index(-1);
  std::vector<Index> jump_pcs;
  int const          last_index = static_cast<int>(node->children.size()) - 1;

  for (int i = 0; i <= last_index; ++i)
  {
    BlockNodePtr block = ConvertToBlockNodePtr(node->children[std::size_t(i)]);
    if (block->kind != Node::Kind::Else)
    {
      // block is the if block or one of the elseif blocks
      Index const condition_pc = (Index)function_->instructions.size();

      if (jf_pc != Index(-1))
      {
        // Previous block, if there is one, jumps to here
        function_->instructions[jf_pc].index = condition_pc;
      }

      ExpressionNodePtr condition_expression = ConvertToExpressionNodePtr(block->children[0]);
      HandleExpression(condition_expression);

      Script::Instruction jf_instruction(Opcodes::JumpIfFalse, condition_expression->token.line);
      jf_instruction.index = 0;  // pc placeholder
      jf_pc                = function_->AddInstruction(jf_instruction);

      ScopeEnter();
      HandleBlock(block);
      ScopeLeave(block);

      if (i < last_index)
      {
        // Only arrange jump to the endif if not the last block
        Script::Instruction jump_instruction(Opcodes::Jump, block->block_terminator.line);
        jump_instruction.index = 0;  // pc placeholder
        Index const jump_pc    = function_->AddInstruction(jump_instruction);
        jump_pcs.push_back(jump_pc);
      }
    }
    else
    {
      // block is the else block
      Index const else_block_start_pc = static_cast<Index>(function_->instructions.size());

      function_->instructions[jf_pc].index = else_block_start_pc;
      jf_pc                                = Index(-1);

      ScopeEnter();
      HandleBlock(block);
      ScopeLeave(block);
    }
  }

  Index const endif_pc = (Index)function_->instructions.size();

  if (jf_pc != Index(-1))
  {
    function_->instructions[jf_pc].index = endif_pc;
  }

  for (auto jump_pc : jump_pcs)
  {
    function_->instructions[jump_pc].index = endif_pc;
  }
}

void Generator::HandleVarStatement(NodePtr const &node)
{
  ExpressionNodePtr identifier_node = ConvertToExpressionNodePtr(node->children[0]);
  VariablePtr       v               = identifier_node->variable;
  v->index                          = function_->AddVariable(v->name, v->type->id);
  bool const is_object              = v->type->category != TypeCategory::Primitive;
  int        scope_number;

  if (is_object)
  {
    scope_number = static_cast<int>(scopes_.size()) - 1;
    Scope &scope = scopes_[std::size_t(scope_number)];
    scope.objects.push_back(v->index);
  }
  else
  {
    // Primitives don't need to be destructed on scope exit
    scope_number = -1;
  }

  if (node->kind == Node::Kind::VarDeclarationStatement)
  {
    Script::Instruction instruction(Opcodes::VarDeclare, node->token.line);
    instruction.index    = v->index;
    instruction.type_id  = v->type->id;
    instruction.data.i32 = scope_number;
    function_->AddInstruction(instruction);
  }
  else
  {
    ExpressionNodePtr rhs;
    if (node->kind == Node::Kind::VarDeclarationTypedAssignmentStatement)
    {
      rhs = ConvertToExpressionNodePtr(node->children[2]);
    }
    else
    {
      rhs = ConvertToExpressionNodePtr(node->children[1]);
    }

    HandleExpression(rhs);
    Script::Instruction instruction(Opcodes::VarDeclareAssign, node->token.line);
    instruction.index    = v->index;
    instruction.type_id  = v->type->id;
    instruction.data.i32 = scope_number;
    function_->AddInstruction(instruction);
  }
}

void Generator::HandleReturnStatement(NodePtr const &node)
{
  if (node->children.size() == 0)
  {
    Script::Instruction instruction(Opcodes::Return, node->token.line);
    instruction.type_id  = TypeIds::Void;
    instruction.data.i32 = 0;  // scope number
    function_->AddInstruction(instruction);
    return;
  }

  ExpressionNodePtr expression = ConvertToExpressionNodePtr(node->children[0]);
  HandleExpression(expression);

  Script::Instruction instruction(Opcodes::ReturnValue, node->token.line);
  instruction.type_id  = expression->type->id;
  instruction.data.i32 = 0;  // scope number

  function_->AddInstruction(instruction);
}

void Generator::HandleBreakStatement(NodePtr const &node)
{
  Loop &loop = loops_.back();

  Script::Instruction instruction(Opcodes::Break, node->token.line);
  instruction.index    = 0;  // pc placeholder
  instruction.data.i32 = loop.scope_number;
  Index const break_pc = function_->AddInstruction(instruction);
  loop.break_pcs.push_back(break_pc);
}

void Generator::HandleContinueStatement(NodePtr const &node)
{
  Loop &              loop = loops_.back();
  Script::Instruction instruction(Opcodes::Continue, node->token.line);
  instruction.index       = 0;  // pc placeholder
  instruction.data.i32    = loop.scope_number;
  Index const continue_pc = function_->AddInstruction(instruction);
  loop.continue_pcs.push_back(continue_pc);
}

Opcode Generator::GetArithmeticAssignmentOpcode(bool assigning_to_variable, bool lhs_is_primitive,
                                                bool rhs_is_primitive, Opcode opcode1,
                                                Opcode opcode2, Opcode opcode3, Opcode opcode4,
                                                Opcode opcode5, Opcode opcode6)
{
  Opcode opcode;
  if (assigning_to_variable)
  {
    if (lhs_is_primitive)
    {
      opcode = opcode1;
    }
    else if (rhs_is_primitive)
    {
      opcode = opcode2;
    }
    else
    {
      opcode = opcode3;
    }
  }
  else  // assigning to element
  {
    if (lhs_is_primitive)
    {
      opcode = opcode4;
    }
    else if (rhs_is_primitive)
    {
      opcode = opcode5;
    }
    else
    {
      opcode = opcode6;
    }
  }
  return opcode;
}

void Generator::HandleAssignmentStatement(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs                   = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs                   = ConvertToExpressionNodePtr(node->children[1]);
  bool const        assigning_to_variable = lhs->category == ExpressionNode::Category::Variable;
  // bool const assigning_to_element = lhs->kind == Node::Kind::IndexOp;
  Opcode     opcode           = Opcodes::Unknown;
  bool const lhs_is_primitive = lhs->type->category == TypeCategory::Primitive;
  bool const rhs_is_primitive = rhs->type->category == TypeCategory::Primitive;

  switch (node->kind)
  {
  case Node::Kind::AssignOp:
  {
    if (assigning_to_variable)
    {
      opcode = Opcodes::PopToVariable;
    }
    else  // assigning_to_element
    {
      opcode = Opcodes::PopToElement;
    }
    break;
  }
  case Node::Kind::ModuloAssignOp:
  {
    if (assigning_to_variable)
    {
      opcode = Opcodes::VariableModuloAssign;
    }
    else  // assigning to element
    {
      opcode = Opcodes::ElementModuloAssign;
    }
    break;
  }
  case Node::Kind::AddAssignOp:
  {
    opcode = GetArithmeticAssignmentOpcode(
        assigning_to_variable, lhs_is_primitive, rhs_is_primitive, Opcodes::VariableAddAssign,
        Opcodes::VariableRightAddAssign, Opcodes::VariableObjectAddAssign,
        Opcodes::ElementAddAssign, Opcodes::ElementRightAddAssign, Opcodes::ElementObjectAddAssign);
    break;
  }
  case Node::Kind::SubtractAssignOp:
  {
    opcode = GetArithmeticAssignmentOpcode(
        assigning_to_variable, lhs_is_primitive, rhs_is_primitive, Opcodes::VariableSubtractAssign,
        Opcodes::VariableRightSubtractAssign, Opcodes::VariableObjectSubtractAssign,
        Opcodes::ElementSubtractAssign, Opcodes::ElementRightSubtractAssign,
        Opcodes::ElementObjectSubtractAssign);
    break;
  }
  case Node::Kind::MultiplyAssignOp:
  {
    opcode = GetArithmeticAssignmentOpcode(
        assigning_to_variable, lhs_is_primitive, rhs_is_primitive, Opcodes::VariableMultiplyAssign,
        Opcodes::VariableRightMultiplyAssign, Opcodes::VariableObjectMultiplyAssign,
        Opcodes::ElementMultiplyAssign, Opcodes::ElementRightMultiplyAssign,
        Opcodes::ElementObjectMultiplyAssign);
    break;
  }
  case Node::Kind::DivideAssignOp:
  {
    opcode = GetArithmeticAssignmentOpcode(
        assigning_to_variable, lhs_is_primitive, rhs_is_primitive, Opcodes::VariableDivideAssign,
        Opcodes::VariableRightDivideAssign, Opcodes::VariableObjectDivideAssign,
        Opcodes::ElementDivideAssign, Opcodes::ElementRightDivideAssign,
        Opcodes::ElementObjectDivideAssign);
    break;
  }
  default:
  {
    break;
  }
  }  // switch
  HandleAssignment(lhs, opcode, rhs);
}

void Generator::HandleAssignment(ExpressionNodePtr const &lhs, Opcode opcode,
                                 ExpressionNodePtr const &rhs)
{
  bool const assigning_to_variable = lhs->category == ExpressionNode::Category::Variable;
  // bool const assigning_to_element = lhs->kind == Node::Kind::IndexOp;
  if (assigning_to_variable)
  {
    VariablePtr v = lhs->variable;
    if (opcode == Opcodes::PopToVariable)
    {
      HandleExpression(rhs);
      Script::Instruction instruction(opcode, lhs->token.line);
      instruction.index   = v->index;
      instruction.type_id = v->type->id;
      function_->AddInstruction(instruction);
    }
    else
    {
      if (rhs)
      {
        HandleExpression(rhs);
      }
      Script::Instruction instruction(opcode, lhs->token.line);
      instruction.index   = v->index;
      instruction.type_id = v->type->id;
      function_->AddInstruction(instruction);
    }
  }
  else  // assigning to element
  {
    if (rhs)
    {
      HandleExpression(rhs);
    }
    size_t const num_indices     = lhs->children.size() - 1;
    TypeId       element_type_id = lhs->type->id;
    // Arrange for the indices to be pushed on to the stack
    for (size_t i = 1; i <= num_indices; ++i)
    {
      HandleExpression(ConvertToExpressionNodePtr(lhs->children[i]));
    }
    ExpressionNodePtr container_node = ConvertToExpressionNodePtr(lhs->children[0]);
    // Arrange for the container object to be pushed on to the stack
    HandleExpression(container_node);
    Script::Instruction instruction(opcode, lhs->token.line);
    instruction.type_id = element_type_id;
    function_->AddInstruction(instruction);
  }
}

void Generator::HandleExpression(ExpressionNodePtr const &node)
{
  switch (node->kind)
  {
  case Node::Kind::Identifier:
  {
    HandleIdentifier(node);
    break;
  }
  case Node::Kind::Integer8:
  {
    HandleInteger8(node);
    break;
  }
  case Node::Kind::UnsignedInteger8:
  {
    HandleUnsignedInteger8(node);
    break;
  }
  case Node::Kind::Integer16:
  {
    HandleInteger16(node);
    break;
  }
  case Node::Kind::UnsignedInteger16:
  {
    HandleUnsignedInteger16(node);
    break;
  }
  case Node::Kind::Integer32:
  {
    HandleInteger32(node);
    break;
  }
  case Node::Kind::UnsignedInteger32:
  {
    HandleUnsignedInteger32(node);
    break;
  }
  case Node::Kind::Integer64:
  {
    HandleInteger64(node);
    break;
  }
  case Node::Kind::UnsignedInteger64:
  {
    HandleUnsignedInteger64(node);
    break;
  }
  case Node::Kind::Float32:
  {
    HandleFloat32(node);
    break;
  }
  case Node::Kind::Float64:
  {
    HandleFloat64(node);
    break;
  }
  case Node::Kind::String:
  {
    HandleString(node);
    break;
  }
  case Node::Kind::True:
  {
    HandleTrue(node);
    break;
  }
  case Node::Kind::False:
  {
    HandleFalse(node);
    break;
  }
  case Node::Kind::Null:
  {
    HandleNull(node);
    break;
  }
  case Node::Kind::PrefixIncOp:
  case Node::Kind::PrefixDecOp:
  case Node::Kind::PostfixIncOp:
  case Node::Kind::PostfixDecOp:
  {
    HandleIncDecOp(node);
    break;
  }
  case Node::Kind::ModuloOp:
  case Node::Kind::AddOp:
  case Node::Kind::SubtractOp:
  case Node::Kind::MultiplyOp:
  case Node::Kind::DivideOp:
  case Node::Kind::EqualOp:
  case Node::Kind::NotEqualOp:
  case Node::Kind::LessThanOp:
  case Node::Kind::LessThanOrEqualOp:
  case Node::Kind::GreaterThanOp:
  case Node::Kind::GreaterThanOrEqualOp:
  case Node::Kind::AndOp:
  case Node::Kind::OrOp:
  {
    HandleBinaryOp(node);
    break;
  }
  case Node::Kind::UnaryMinusOp:
  case Node::Kind::NotOp:
  {
    HandleUnaryOp(node);
    break;
  }
  case Node::Kind::IndexOp:
  {
    HandleIndexOp(node);
    break;
  }
  case Node::Kind::DotOp:
  {
    HandleDotOp(node);
    break;
  }
  case Node::Kind::InvokeOp:
  {
    HandleInvokeOp(node);
    break;
  }
  default:
  {
    break;
  }
  }  // switch
}

void Generator::HandleIdentifier(ExpressionNodePtr const &node)
{
  VariablePtr         v = node->variable;
  Script::Instruction instruction(Opcodes::PushVariable, node->token.line);
  instruction.index   = v->index;
  instruction.type_id = v->type->id;
  function_->AddInstruction(instruction);
}

void Generator::HandleInteger8(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id = TypeIds::Int8;
  instruction.data.i8 = static_cast<int8_t>(atoi(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleUnsignedInteger8(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id  = TypeIds::Byte;
  instruction.data.ui8 = static_cast<uint8_t>(atoi(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleInteger16(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id  = TypeIds::Int16;
  instruction.data.i16 = static_cast<int16_t>(atol(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleUnsignedInteger16(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id   = TypeIds::UInt16;
  instruction.data.ui16 = static_cast<uint16_t>(atol(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleInteger32(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id  = TypeIds::Int32;
  instruction.data.i32 = static_cast<int32_t>(atoi(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleUnsignedInteger32(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id   = TypeIds::UInt32;
  instruction.data.ui32 = static_cast<uint32_t>(atoll(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleInteger64(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id  = TypeIds::Int64;
  instruction.data.i64 = static_cast<int64_t>(atoll(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleUnsignedInteger64(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id   = TypeIds::UInt64;
  instruction.data.ui64 = static_cast<uint64_t>(atoll(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleFloat32(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id  = TypeIds::Float32;
  instruction.data.f32 = (float)atof(node->token.text.c_str());
  function_->AddInstruction(instruction);
}

void Generator::HandleFloat64(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id  = TypeIds::Float64;
  instruction.data.f64 = atof(node->token.text.c_str());
  function_->AddInstruction(instruction);
}

void Generator::HandleString(ExpressionNodePtr const &node)
{
  std::string s = node->token.text.substr(1, node->token.text.size() - 2);
  Index       index;
  auto        it = strings_map_.find(s);
  if (it != strings_map_.end())
  {
    index = it->second;
  }
  else
  {
    index = (Index)strings_.size();
    strings_.push_back(s);
    strings_map_[s] = index;
  }

  Script::Instruction instruction(Opcodes::PushString, node->token.line);
  instruction.index   = index;
  instruction.type_id = TypeIds::String;
  function_->AddInstruction(instruction);
}

void Generator::HandleTrue(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id  = TypeIds::Bool;
  instruction.data.ui8 = 1;
  function_->AddInstruction(instruction);
}

void Generator::HandleFalse(ExpressionNodePtr const &node)
{
  Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
  instruction.type_id  = TypeIds::Bool;
  instruction.data.ui8 = 0;
  function_->AddInstruction(instruction);
}

void Generator::HandleNull(ExpressionNodePtr const &node)
{
  if (node->type->id != TypeIds::Null)
  {
    Script::Instruction instruction(Opcodes::PushNull, node->token.line);
    instruction.type_id = node->type->id;
    function_->AddInstruction(instruction);
  }
  else
  {
    // Type-uninferable nulls (e.g. in "null == null") are transformed to integral zero
    Script::Instruction instruction(Opcodes::PushConstant, node->token.line);
    instruction.type_id = TypeIds::UInt64;
    instruction.data.Zero();
    function_->AddInstruction(instruction);
  }
}

void Generator::HandleIncDecOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr operand               = ConvertToExpressionNodePtr(node->children[0]);
  bool const        assigning_to_variable = operand->category == ExpressionNode::Category::Variable;
  // bool const assigning_to_element = lhs->kind == Node::Kind::IndexOp;
  Opcode opcode = Opcodes::Unknown;
  if (node->kind == Node::Kind::PrefixIncOp)
  {
    opcode = assigning_to_variable ? Opcodes::VariablePrefixInc : Opcodes::ElementPrefixInc;
  }
  else if (node->kind == Node::Kind::PrefixDecOp)
  {
    opcode = assigning_to_variable ? Opcodes::VariablePrefixDec : Opcodes::ElementPrefixDec;
  }
  else if (node->kind == Node::Kind::PostfixIncOp)
  {
    opcode = assigning_to_variable ? Opcodes::VariablePostfixInc : Opcodes::ElementPostfixInc;
  }
  else if (node->kind == Node::Kind::PostfixDecOp)
  {
    opcode = assigning_to_variable ? Opcodes::VariablePostfixDec : Opcodes::ElementPostfixDec;
  }
  HandleAssignment(operand, opcode, nullptr);
}

Opcode Generator::GetArithmeticOpcode(bool lhs_is_primitive, bool rhs_is_primitive, Opcode opcode1,
                                      Opcode opcode2, Opcode opcode3, Opcode opcode4)
{
  Opcode opcode;
  if (lhs_is_primitive)
  {
    if (rhs_is_primitive)
    {
      // primitive op primitive
      opcode = opcode1;
    }
    else
    {
      // primitive op object
      opcode = opcode2;
    }
  }
  else
  {
    if (rhs_is_primitive)
    {
      // object op primitive
      opcode = opcode3;
    }
    else
    {
      // object op object
      opcode = opcode4;
    }
  }
  return opcode;
}

void Generator::HandleBinaryOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs              = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs              = ConvertToExpressionNodePtr(node->children[1]);
  Opcode            opcode           = Opcodes::Unknown;
  TypeId            type_id          = TypeIds::Unknown;
  bool const        lhs_is_primitive = lhs->type->category == TypeCategory::Primitive;
  bool const        rhs_is_primitive = rhs->type->category == TypeCategory::Primitive;

  switch (node->kind)
  {
  case Node::Kind::ModuloOp:
  {
    opcode  = Opcodes::Modulo;
    type_id = node->type->id;
    break;
  }
  case Node::Kind::AddOp:
  {
    opcode = GetArithmeticOpcode(lhs_is_primitive, rhs_is_primitive, Opcodes::Add, Opcodes::LeftAdd,
                                 Opcodes::RightAdd, Opcodes::ObjectAdd);
    type_id = node->type->id;
    break;
  }
  case Node::Kind::SubtractOp:
  {
    opcode =
        GetArithmeticOpcode(lhs_is_primitive, rhs_is_primitive, Opcodes::Subtract,
                            Opcodes::LeftSubtract, Opcodes::RightSubtract, Opcodes::ObjectSubtract);
    type_id = node->type->id;
    break;
  }
  case Node::Kind::MultiplyOp:
  {
    opcode =
        GetArithmeticOpcode(lhs_is_primitive, rhs_is_primitive, Opcodes::Multiply,
                            Opcodes::LeftMultiply, Opcodes::RightMultiply, Opcodes::ObjectMultiply);
    type_id = node->type->id;
    break;
  }
  case Node::Kind::DivideOp:
  {
    opcode  = GetArithmeticOpcode(lhs_is_primitive, rhs_is_primitive, Opcodes::Divide,
                                 Opcodes::LeftDivide, Opcodes::RightDivide, Opcodes::ObjectDivide);
    type_id = node->type->id;
    break;
  }
  case Node::Kind::EqualOp:
  {
    opcode = lhs_is_primitive ? Opcodes::Equal : Opcodes::ObjectEqual;
    if ((lhs->type->id == TypeIds::Null) && (rhs->type->id == TypeIds::Null))
    {
      // Type-uninferable nulls (e.g. in "null == null") are transformed to integral zero
      type_id = TypeIds::UInt64;
    }
    else
    {
      type_id = lhs->type->id;
    }
    break;
  }
  case Node::Kind::NotEqualOp:
  {
    opcode = lhs_is_primitive ? Opcodes::NotEqual : Opcodes::ObjectNotEqual;
    if ((lhs->type->id == TypeIds::Null) && (rhs->type->id == TypeIds::Null))
    {
      // Type-uninferable nulls (e.g. in "null == null") are transformed to integral zero
      type_id = TypeIds::UInt64;
    }
    else
    {
      type_id = lhs->type->id;
    }
    break;
  }
  case Node::Kind::LessThanOp:
  {
    opcode  = lhs_is_primitive ? Opcodes::LessThan : Opcodes::ObjectLessThan;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::LessThanOrEqualOp:
  {
    opcode  = lhs_is_primitive ? Opcodes::LessThanOrEqual : Opcodes::ObjectLessThanOrEqual;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::GreaterThanOp:
  {
    opcode  = lhs_is_primitive ? Opcodes::GreaterThan : Opcodes::ObjectGreaterThan;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::GreaterThanOrEqualOp:
  {
    opcode  = lhs_is_primitive ? Opcodes::GreaterThanOrEqual : Opcodes::ObjectGreaterThanOrEqual;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::AndOp:
  {
    opcode = Opcodes::And;
    // This will be TypeId::Bool
    type_id = node->type->id;
    break;
  }
  case Node::Kind::OrOp:
  {
    opcode = Opcodes::Or;
    // This will be TypeId::Bool
    type_id = node->type->id;
    break;
  }
  default:
  {
    break;
  }
  }  // switch

  for (size_t i = 0; i < node->children.size(); ++i)
  {
    HandleExpression(ConvertToExpressionNodePtr(node->children[i]));
  }
  Script::Instruction instruction(opcode, node->token.line);
  instruction.type_id = type_id;
  function_->AddInstruction(instruction);
}

void Generator::HandleUnaryOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr operand      = ConvertToExpressionNodePtr(node->children[0]);
  Opcode            opcode       = Opcodes::Unknown;
  TypeId            type_id      = TypeIds::Unknown;
  bool const        is_primitive = operand->type->category == TypeCategory::Primitive;

  switch (node->kind)
  {
  case Node::Kind::UnaryMinusOp:
  {
    opcode  = is_primitive ? Opcodes::UnaryMinus : Opcodes::ObjectUnaryMinus;
    type_id = node->type->id;
    break;
  }
  case Node::Kind::NotOp:
  {
    opcode = Opcodes::Not;
    // This will be TypeId::Bool
    type_id = node->type->id;
    break;
  }
  default:
  {
    break;
  }
  }  // switch

  HandleExpression(operand);
  Script::Instruction instruction(opcode, node->token.line);
  instruction.type_id = type_id;
  function_->AddInstruction(instruction);
}

void Generator::HandleIndexOp(ExpressionNodePtr const &node)
{
  size_t const num_indices = node->children.size() - 1;
  // Arrange for the indices to be pushed on to the stack
  for (size_t i = 1; i <= num_indices; ++i)
  {
    HandleExpression(ConvertToExpressionNodePtr(node->children[i]));
  }
  ExpressionNodePtr container_node = ConvertToExpressionNodePtr(node->children[0]);
  // Arrange for the container object to be pushed on to the stack
  HandleExpression(container_node);
  Script::Instruction instruction(Opcodes::PushElement, node->token.line);
  TypeId              element_type_id = node->type->id;
  instruction.type_id                 = element_type_id;
  function_->AddInstruction(instruction);
}

void Generator::HandleDotOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  if ((lhs->category == ExpressionNode::Category::Variable) ||
      (lhs->category == ExpressionNode::Category::LV) ||
      (lhs->category == ExpressionNode::Category::RV))
  {
    // Arrange for the instance object to be pushed on to the stack
    HandleExpression(ConvertToExpressionNodePtr(lhs));
  }
}

void Generator::HandleInvokeOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  FunctionPtr       f   = node->function;
  if (f->kind == Function::Kind::OpcodeInstanceFunction)
  {
    // Arrange for the instance object to be pushed on to the stack
    HandleExpression(ConvertToExpressionNodePtr(lhs));
  }
  // Arrange for the function parameters to be pushed on to the stack
  for (size_t i = 1; i < node->children.size(); ++i)
  {
    HandleExpression(ConvertToExpressionNodePtr(node->children[i]));
  }
  if (f->kind == Function::Kind::UserFreeFunction)
  {
    Script::Instruction instruction(Opcodes::InvokeUserFunction, node->token.line);
    instruction.index   = f->index;
    instruction.type_id = node->type->id;
    function_->AddInstruction(instruction);
  }
  else
  {
    // Opcode-invoked free function
    // Opcode-invoked type constructor
    // Opcode-invoked type function
    // Opcode-invoked instance function
    Script::Instruction instruction(f->opcode, node->token.line);
    // Store the type id of the return type
    instruction.type_id = node->type->id;
    if (lhs->type)
    {
      // Store the type id of the invoking type
      instruction.data.ui16 = lhs->type->id;
    }
    else
    {
      instruction.data.ui16 = TypeIds::Unknown;
    }
    function_->AddInstruction(instruction);
  }
}

void Generator::ScopeEnter()
{
  scopes_.push_back(Scope());
}

void Generator::ScopeLeave(BlockNodePtr block_node)
{
  int const scope_number = (int)scopes_.size() - 1;
  Scope &   scope        = scopes_[std::size_t(scope_number)];
  if (scope.objects.size() > 0)
  {
    // Note: the function definition block does not require a Destruct instruction
    // because the Return and ReturnValue instructions, which must always be
    // present, already take care of destructing objects
    if (block_node->kind != Node::Kind::FunctionDefinitionStatement)
    {
      Script::Instruction instruction(Opcodes::Destruct, block_node->block_terminator.line);
      // Arrange for all live objects with scope >= scope_number to be destructed
      instruction.data.i32 = scope_number;
      function_->AddInstruction(instruction);
    }
  }
  scopes_.pop_back();
}

}  // namespace vm
}  // namespace fetch
