#include "vm/generator.hpp"
#include <sstream>

namespace fetch {
namespace vm {

void Generator::Generate(const BlockNodePtr &root, const std::string &name, Script &script)
{
  script_ = Script(name);
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

void Generator::CreateFunctions(const BlockNodePtr &root)
{
  for (const NodePtr &child : root->block_children)
  {
    switch (child->kind)
    {
    case Node::Kind::FunctionDefinitionStatement:
    {
      BlockNodePtr      function_definition_node = ConvertToBlockNodePtr(child);
      ExpressionNodePtr identifier_node =
          ConvertToExpressionNodePtr(function_definition_node->children[0]);
      FunctionPtr        f              = identifier_node->function;
      const std::string &name           = f->name;
      const int          num_parameters = (int)f->parameter_variables.size();
      Script::Function   function(name, num_parameters);
      for (const VariablePtr &v : f->parameter_variables)
        v->index = function.AddVariable(v->name, v->type->id);
      f->index = script_.AddFunction(function);
      break;
    }
    default:
      break;
    }
  }
}

void Generator::HandleBlock(const BlockNodePtr &block)
{
  for (const NodePtr &child : block->block_children)
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
      if (expression->type->id != TypeId::Void)
      {
        // The result of the expression is not consumed, so issue an
        // instruction that will pop it and throw it away
        Script::Instruction instruction(Opcode::Discard, expression->token.line);
        instruction.type_id = expression->type->id;
        function_->AddInstruction(instruction);
      }
      break;
    }
    }
  }
}

void Generator::HandleFunctionDefinitionStatement(const BlockNodePtr &node)
{
  ExpressionNodePtr identifier_node = ConvertToExpressionNodePtr(node->children[0]);
  FunctionPtr       f               = identifier_node->function;
  function_                         = &(script_.functions[f->index]);
  ScopeEnter();
  HandleBlock(node);
  ScopeLeave(node);
  if (f->return_type->id == TypeId::Void)
  {
    // 0 is not correct line number, it should be the line number of the endfunction token
    Script::Instruction instruction(Opcode::Return, 0);
    instruction.type_id     = TypeId::Void;
    instruction.variant.i32 = 0;  // scope number
    function_->AddInstruction(instruction);
  }
  function_ = nullptr;
}

void Generator::HandleWhileStatement(const BlockNodePtr &node)
{
  const Index       continue_pc = (Index)function_->instructions.size();
  ExpressionNodePtr condition   = ConvertToExpressionNodePtr(node->children[0]);
  HandleExpression(condition);
  const Index         jf_pc = (Index)function_->instructions.size();
  Script::Instruction jf_instruction(Opcode::JumpIfFalse, node->token.line);
  jf_instruction.index = 0;  // pc placeholder
  function_->AddInstruction(jf_instruction);
  ScopeEnter();
  const int scope_number = (int)scopes_.size() - 1;
  loops_.push_back(Loop());
  loops_.back().scope_number = scope_number;
  HandleBlock(node);
  ScopeLeave(node);
  // 0 is not correct line number, it should be the line number of the endwhile token
  Script::Instruction jump_instruction(Opcode::Jump, 0);
  jump_instruction.index = continue_pc;
  function_->AddInstruction(jump_instruction);
  const Index endwhile_pc              = (Index)function_->instructions.size();
  function_->instructions[jf_pc].index = endwhile_pc;
  Loop &loop                           = loops_.back();
  for (auto jump_pc : loop.continue_pcs) function_->instructions[jump_pc].index = continue_pc;
  for (auto jump_pc : loop.break_pcs) function_->instructions[jump_pc].index = endwhile_pc;
  loops_.pop_back();
}

void Generator::HandleForStatement(const BlockNodePtr &node)
{
  ExpressionNodePtr identifier_node = ConvertToExpressionNodePtr(node->children[0]);
  VariablePtr       v               = identifier_node->variable;
  const int         arity           = (int)node->children.size() - 1;
  for (int i = 1; i <= arity; ++i)
    HandleExpression(ConvertToExpressionNodePtr(node->children[std::size_t(i)]));
  // Add the for-loop variable
  v->index = function_->AddVariable(v->name, v->type->id);
  Script::Instruction init_instruction(Opcode::ForRangeInit, node->token.line);
  init_instruction.index       = v->index;  // frame-relative index of the for-loop variable
  init_instruction.type_id     = v->type->id;
  init_instruction.variant.i32 = arity;  // 2 or 3 range elements
  function_->AddInstruction(init_instruction);
  Script::Instruction iterate_instruction(Opcode::ForRangeIterate, node->token.line);
  iterate_instruction.index       = 0;      // pc placeholder
  iterate_instruction.variant.i32 = arity;  // 2 or 3 range elements
  const Index iterate_pc          = function_->AddInstruction(iterate_instruction);
  ScopeEnter();
  const int scope_number = (int)scopes_.size() - 1;
  loops_.push_back(Loop());
  loops_.back().scope_number = scope_number;
  HandleBlock(node);
  ScopeLeave(node);
  // 0 is not correct line number, it should be the line number of the endfor token
  Script::Instruction jump_instruction(Opcode::Jump, 0);
  jump_instruction.index = iterate_pc;
  function_->AddInstruction(jump_instruction);
  // 0 is not correct line number, it should be the line number of the endfor token
  Script::Instruction terminate_instruction(Opcode::ForRangeTerminate, 0);
  terminate_instruction.index   = v->index;
  terminate_instruction.type_id = v->type->id;
  const Index terminate_pc      = function_->AddInstruction(terminate_instruction);
  Loop &      loop              = loops_.back();
  for (auto jump_pc : loop.continue_pcs) function_->instructions[jump_pc].index = iterate_pc;
  for (auto jump_pc : loop.break_pcs) function_->instructions[jump_pc].index = terminate_pc;
  function_->instructions[iterate_pc].index = terminate_pc;
  loops_.pop_back();
}

void Generator::HandleIfStatement(const NodePtr &node)
{
  Index              jf_pc = Index(-1);
  std::vector<Index> jump_pcs;
  const int          last_index = (int)node->children.size() - 1;
  for (int i = 0; i <= last_index; ++i)
  {
    BlockNodePtr block = ConvertToBlockNodePtr(node->children[std::size_t(i)]);
    if (block->kind != Node::Kind::Else)
    {
      // block is the if block or one of the elseif blocks
      const Index condition_pc = (Index)function_->instructions.size();
      if (jf_pc != Index(-1))
      {
        // Previous block, if there is one, jumps to here
        function_->instructions[jf_pc].index = condition_pc;
      }
      ExpressionNodePtr condition_expression = ConvertToExpressionNodePtr(block->children[0]);
      HandleExpression(condition_expression);
      Script::Instruction jf_instruction(Opcode::JumpIfFalse, condition_expression->token.line);
      jf_instruction.index = 0;  // pc placeholder
      jf_pc                = function_->AddInstruction(jf_instruction);
      ScopeEnter();
      HandleBlock(block);
      ScopeLeave(block);
      if (i < last_index)
      {
        // Only arrange jump to the endif if not the last block
        // 0 is not correct line number!
        Script::Instruction jump_instruction(Opcode::Jump, 0);
        jump_instruction.index = 0;  // pc placeholder
        const Index jump_pc    = function_->AddInstruction(jump_instruction);
        jump_pcs.push_back(jump_pc);
      }
    }
    else
    {
      // block is the else block
      const Index else_block_start_pc      = (Index)function_->instructions.size();
      function_->instructions[jf_pc].index = else_block_start_pc;
      jf_pc                                = Index(-1);
      ScopeEnter();
      HandleBlock(block);
      ScopeLeave(block);
    }
  }
  const Index endif_pc = (Index)function_->instructions.size();
  if (jf_pc != Index(-1)) function_->instructions[jf_pc].index = endif_pc;
  for (auto jump_pc : jump_pcs) function_->instructions[jump_pc].index = endif_pc;
}

void Generator::HandleVarStatement(const NodePtr &node)
{
  ExpressionNodePtr identifier_node = ConvertToExpressionNodePtr(node->children[0]);
  VariablePtr       v               = identifier_node->variable;
  v->index                          = function_->AddVariable(v->name, v->type->id);
  const bool is_object              = v->type->IsPrimitiveType() == false;
  int        scope_number;
  if (is_object)
  {
    scope_number = (int)scopes_.size() - 1;
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
    Script::Instruction instruction(Opcode::VarDeclare, node->token.line);
    instruction.index       = v->index;
    instruction.variant.i32 = scope_number;
    instruction.type_id     = v->type->id;
    function_->AddInstruction(instruction);
  }
  else
  {
    ExpressionNodePtr rhs;
    if (node->kind == Node::Kind::VarDeclarationTypedAssignmentStatement)
      rhs = ConvertToExpressionNodePtr(node->children[2]);
    else
      rhs = ConvertToExpressionNodePtr(node->children[1]);
    HandleExpression(rhs);
    Script::Instruction instruction(Opcode::VarDeclareAssign, node->token.line);
    instruction.index       = v->index;
    instruction.variant.i32 = scope_number;
    instruction.type_id     = v->type->id;
    function_->AddInstruction(instruction);
  }
}

void Generator::HandleReturnStatement(const NodePtr &node)
{
  if (node->children.size() == 0)
  {
    Script::Instruction instruction(Opcode::Return, node->token.line);
    instruction.type_id     = TypeId::Void;
    instruction.variant.i32 = 0;  // scope number
    function_->AddInstruction(instruction);
    return;
  }
  ExpressionNodePtr expression = ConvertToExpressionNodePtr(node->children[0]);
  HandleExpression(expression);
  Script::Instruction instruction(Opcode::ReturnValue, node->token.line);
  instruction.type_id     = expression->type->id;
  instruction.variant.i32 = 0;  // scope number
  function_->AddInstruction(instruction);
}

void Generator::HandleBreakStatement(const NodePtr &node)
{
  Loop &              loop = loops_.back();
  Script::Instruction instruction(Opcode::Break, node->token.line);
  instruction.index       = 0;  // pc placeholder
  instruction.variant.i32 = loop.scope_number;
  const Index break_pc    = function_->AddInstruction(instruction);
  loop.break_pcs.push_back(break_pc);
}

void Generator::HandleContinueStatement(const NodePtr &node)
{
  Loop &              loop = loops_.back();
  Script::Instruction instruction(Opcode::Continue, node->token.line);
  instruction.index       = 0;  // pc placeholder
  instruction.variant.i32 = loop.scope_number;
  const Index continue_pc = function_->AddInstruction(instruction);
  loop.continue_pcs.push_back(continue_pc);
}

void Generator::HandleAssignmentStatement(const ExpressionNodePtr &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  HandleExpression(rhs);
  const bool assigning_to_variable      = lhs->category == ExpressionNode::Category::Variable;
  const bool assigning_to_indexed_value = lhs->kind == Node::Kind::IndexOp;
  const bool is_compound_assignment     = node->kind != Node::Kind::AssignOp;
  Opcode     opcode_override            = Opcode::Unknown;
  if (assigning_to_variable)
  {
    if (is_compound_assignment == false)
    {
      opcode_override = Opcode::Assign;
    }
    else
    {
      switch (node->kind)
      {
      case Node::Kind::AddAssignOp:
      {
        opcode_override = Opcode::AddAssignOp;
        break;
      }
      case Node::Kind::SubtractAssignOp:
      {
        opcode_override = Opcode::SubtractAssignOp;
        break;
      }
      case Node::Kind::MultiplyAssignOp:
      {
        opcode_override = Opcode::MultiplyAssignOp;
        break;
      }
      case Node::Kind::DivideAssignOp:
      {
        opcode_override = Opcode::DivideAssignOp;
        break;
      }
      default:
        break;
      }
    }
    HandleLHSExpression(lhs, opcode_override, rhs);
  }
  else if (assigning_to_indexed_value)
  {
    if (is_compound_assignment == false)
    {
      opcode_override = Opcode::IndexedAssign;
    }
    else
    {
      switch (node->kind)
      {
      case Node::Kind::AddAssignOp:
      {
        opcode_override = Opcode::IndexedAddAssignOp;
        break;
      }
      case Node::Kind::SubtractAssignOp:
      {
        opcode_override = Opcode::IndexedSubtractAssignOp;
        break;
      }
      case Node::Kind::MultiplyAssignOp:
      {
        opcode_override = Opcode::IndexedMultiplyAssignOp;
        break;
      }
      case Node::Kind::DivideAssignOp:
      {
        opcode_override = Opcode::IndexedDivideAssignOp;
        break;
      }
      default:
        break;
      }
    }
    HandleLHSExpression(lhs, opcode_override, rhs);
  }
}

void Generator::HandleLHSExpression(const ExpressionNodePtr &lhs, const Opcode override_opcode,
                                    const ExpressionNodePtr &rhs)
{
  if (lhs->category == ExpressionNode::Category::Variable)
  {
    VariablePtr v = lhs->variable;
    Opcode      opcode;
    TypeId      type_id;
    if (override_opcode == Opcode::Unknown)
    {
      opcode  = Opcode::PushVariable;
      type_id = v->type->id;
    }
    else
    {
      opcode  = override_opcode;
      type_id = v->type->id;
      if (IsMatrixType(lhs->type->id))
      {
        // Handle cases where the RHS type is not the same as the LHS type
        // e.g. matrix *= 100.0
        if (rhs->type->id == TypeId::Float32)
          type_id = TypeId::Matrix_Float32__Float32;
        else if (rhs->type->id == TypeId::Float64)
          type_id = TypeId::Matrix_Float64__Float64;
      }
    }
    Script::Instruction instruction(opcode, lhs->token.line);
    instruction.index   = v->index;
    instruction.type_id = type_id;
    function_->AddInstruction(instruction);
  }
  else  // if (lhs->kind == Node::Kind::IndexOp)
  {
    const int         num_rhs_operands = (int)lhs->children.size() - 1;
    ExpressionNodePtr indexed_node     = ConvertToExpressionNodePtr(lhs->children[0]);
    HandleLHSExpression(indexed_node, Opcode::Unknown, nullptr);
    for (int i = 1; i <= num_rhs_operands; ++i)
      HandleExpression(ConvertToExpressionNodePtr(lhs->children[std::size_t(i)]));
    Opcode opcode;
    TypeId type_id;
    if (override_opcode == Opcode::Unknown)
    {
      opcode = Opcode::IndexOp;
      // This is the type of the matrix or array being indexed
      type_id = indexed_node->type->id;
    }
    else
    {
      opcode = override_opcode;
      // This is the type of the matrix or array being indexed
      type_id = indexed_node->type->id;
      if (IsMatrixType(lhs->type->id))
      {
        // Handle cases where the RHS type is not the same as the element
        // type of the array being indexed e.g.
        // arrayofmatrices[index] *= 100.0
        if (rhs->type->id == TypeId::Float32)
          type_id = TypeId::Array_Matrix_Float32__Float32;
        else if (rhs->type->id == TypeId::Float64)
          type_id = TypeId::Array_Matrix_Float64__Float64;
      }
    }
    Script::Instruction instruction(opcode, lhs->token.line);
    instruction.type_id     = type_id;
    instruction.variant.i32 = num_rhs_operands;
    function_->AddInstruction(instruction);
  }
}

void Generator::HandleExpression(const ExpressionNodePtr &node)
{
  switch (node->kind)
  {
  case Node::Kind::Identifier:
  {
    HandleIdentifier(node);
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
  case Node::Kind::SinglePrecisionNumber:
  {
    HandleSinglePrecisionNumber(node);
    break;
  }
  case Node::Kind::DoublePrecisionNumber:
  {
    HandleDoublePrecisionNumber(node);
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

  case Node::Kind::SquareBracketGroup:
  {
    // ???
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
    HandleOp(node);
    break;
  }
  }
}

void Generator::HandleIdentifier(const ExpressionNodePtr &node)
{
  VariablePtr         v = node->variable;
  Script::Instruction instruction(Opcode::PushVariable, node->token.line);
  instruction.index   = v->index;
  instruction.type_id = v->type->id;
  function_->AddInstruction(instruction);
}

void Generator::HandleInteger32(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id     = TypeId::Int32;
  instruction.variant.i32 = atoi(node->token.text.c_str());
  function_->AddInstruction(instruction);
}

void Generator::HandleUnsignedInteger32(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id      = TypeId::UInt32;
  instruction.variant.ui32 = uint32_t(atoi(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleInteger64(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id     = TypeId::Int64;
  instruction.variant.i64 = atol(node->token.text.c_str());
  function_->AddInstruction(instruction);
}

void Generator::HandleUnsignedInteger64(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id      = TypeId::UInt64;
  instruction.variant.ui64 = uint64_t(atol(node->token.text.c_str()));
  function_->AddInstruction(instruction);
}

void Generator::HandleSinglePrecisionNumber(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id     = TypeId::Float32;
  instruction.variant.f32 = (float)atof(node->token.text.c_str());
  function_->AddInstruction(instruction);
}

void Generator::HandleDoublePrecisionNumber(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id     = TypeId::Float64;
  instruction.variant.f64 = atof(node->token.text.c_str());
  function_->AddInstruction(instruction);
}

void Generator::HandleString(const ExpressionNodePtr &node)
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
  Script::Instruction instruction(Opcode::PushString, node->token.line);
  instruction.index   = index;
  instruction.type_id = TypeId::String;
  function_->AddInstruction(instruction);
}

void Generator::HandleTrue(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id     = TypeId::Bool;
  instruction.variant.ui8 = 1;
  function_->AddInstruction(instruction);
}

void Generator::HandleFalse(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id     = TypeId::Bool;
  instruction.variant.ui8 = 0;
  function_->AddInstruction(instruction);
}

void Generator::HandleNull(const ExpressionNodePtr &node)
{
  Script::Instruction instruction(Opcode::PushConstant, node->token.line);
  instruction.type_id = node->type->id;
  if (node->type->id == TypeId::Null)
  {
    // Unresolved null type...
    // This is to cover the case where there was no possibility of inferring
    // the expected type of the expression involving nulls, e.g. the
    // expression "null == null" -- turn any unconverted nulls into an integral zero
    instruction.type_id = TypeId::UInt64;
  }
  instruction.variant.Zero();
  function_->AddInstruction(instruction);
}

void Generator::HandleIncDecOp(const ExpressionNodePtr &node)
{
  ExpressionNodePtr operand               = ConvertToExpressionNodePtr(node->children[0]);
  const bool        assigning_to_variable = operand->category == ExpressionNode::Category::Variable;
  // const bool assigning_to_indexed_value = operand->kind == Node::Kind::IndexOp;
  Opcode opcode_override = Opcode::Unknown;
  if (node->kind == Node::Kind::PrefixIncOp)
  {
    opcode_override = assigning_to_variable ? Opcode::PrefixIncOp : Opcode::IndexedPrefixIncOp;
  }
  else if (node->kind == Node::Kind::PrefixDecOp)
  {
    opcode_override = assigning_to_variable ? Opcode::PrefixDecOp : Opcode::IndexedPrefixDecOp;
  }
  else if (node->kind == Node::Kind::PostfixIncOp)
  {
    opcode_override = assigning_to_variable ? Opcode::PostfixIncOp : Opcode::IndexedPostfixIncOp;
  }
  else if (node->kind == Node::Kind::PostfixDecOp)
  {
    opcode_override = assigning_to_variable ? Opcode::PostfixDecOp : Opcode::IndexedPostfixDecOp;
  }
  HandleLHSExpression(operand, opcode_override, nullptr);
}

void Generator::HandleOp(const ExpressionNodePtr &node)
{
  ExpressionNodePtr lhs     = ConvertToExpressionNodePtr(node->children[0]);
  Opcode            opcode  = Opcode::Unknown;
  TypeId            type_id = TypeId::Unknown;
  switch (node->kind)
  {
  case Node::Kind::AddOp:
  {
    opcode  = Opcode::AddOp;
    type_id = TestArithmeticTypes(node);
    break;
  }
  case Node::Kind::SubtractOp:
  {
    opcode  = Opcode::SubtractOp;
    type_id = TestArithmeticTypes(node);
    break;
  }
  case Node::Kind::MultiplyOp:
  {
    opcode  = Opcode::MultiplyOp;
    type_id = TestArithmeticTypes(node);
    break;
  }
  case Node::Kind::DivideOp:
  {
    opcode  = Opcode::DivideOp;
    type_id = TestArithmeticTypes(node);
    break;
  }
  case Node::Kind::UnaryMinusOp:
  {
    opcode  = Opcode::UnaryMinusOp;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::EqualOp:
  {
    opcode  = Opcode::EqualOp;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::NotEqualOp:
  {
    opcode  = Opcode::NotEqualOp;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::LessThanOp:
  {
    opcode  = Opcode::LessThanOp;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::LessThanOrEqualOp:
  {
    opcode  = Opcode::LessThanOrEqualOp;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::GreaterThanOp:
  {
    opcode  = Opcode::GreaterThanOp;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::GreaterThanOrEqualOp:
  {
    opcode  = Opcode::GreaterThanOrEqualOp;
    type_id = lhs->type->id;
    break;
  }
  case Node::Kind::AndOp:
  {
    opcode = Opcode::AndOp;
    // This will be TypeId::Bool
    type_id = node->type->id;
    break;
  }
  case Node::Kind::OrOp:
  {
    opcode = Opcode::OrOp;
    // This will be TypeId::Bool
    type_id = node->type->id;
    break;
  }
  case Node::Kind::NotOp:
  {
    opcode = Opcode::NotOp;
    // This will be TypeId::Bool
    type_id = node->type->id;
    break;
  }
  default:
    break;
  }
  for (int i = 0; i < (int)node->children.size(); ++i)
    HandleExpression(ConvertToExpressionNodePtr(node->children[std::size_t(i)]));
  Script::Instruction instruction(opcode, node->token.line);
  instruction.type_id = type_id;
  function_->AddInstruction(instruction);
}

TypeId Generator::TestArithmeticTypes(const ExpressionNodePtr &node)
{
  ExpressionNodePtr lhs         = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs         = ConvertToExpressionNodePtr(node->children[1]);
  const bool        lhs_numeric = IsNumericType(lhs->type->id);
  const bool        rhs_numeric = IsNumericType(rhs->type->id);
  const bool        lhs_matrix  = IsMatrixType(lhs->type->id);
  const bool        rhs_matrix  = IsMatrixType(rhs->type->id);
  if (lhs_numeric && rhs_matrix)
  {
    // Handle e.g. 100.0 * matrix
    if (lhs->type->id == TypeId::Float32)
      return TypeId::Float32__Matrix_Float32;
    else
      return TypeId::Float64__Matrix_Float64;
  }
  else if (lhs_matrix && rhs_numeric)
  {
    // Handle e.g. matrix * 100.0
    if (rhs->type->id == TypeId::Float32)
      return TypeId::Matrix_Float32__Float32;
    else
      return TypeId::Matrix_Float64__Float64;
  }
  else
  {
    return node->type->id;
  }
}

void Generator::HandleIndexOp(const ExpressionNodePtr &node)
{
  const int num_rhs_operands = (int)node->children.size() - 1;
  for (int i = 0; i <= num_rhs_operands; ++i)
    HandleExpression(ConvertToExpressionNodePtr(node->children[std::size_t(i)]));
  Script::Instruction instruction(Opcode::IndexOp, node->token.line);
  ExpressionNodePtr   indexed_node = ConvertToExpressionNodePtr(node->children[0]);
  // This is the type of the matrix or array being indexed
  instruction.type_id     = indexed_node->type->id;
  instruction.variant.i32 = num_rhs_operands;
  function_->AddInstruction(instruction);
}

void Generator::HandleDotOp(const ExpressionNodePtr &node)
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

void Generator::HandleInvokeOp(const ExpressionNodePtr &node)
{
  ExpressionNodePtr lhs            = ConvertToExpressionNodePtr(node->children[0]);
  FunctionPtr       f              = node->function;
  const int         num_parameters = (int)f->parameter_types.size();

  if (f->kind == Function::Kind::OpcodeInstanceFunction)
  {
    // Arrange for the instance object to be pushed on to the stack
    HandleExpression(ConvertToExpressionNodePtr(lhs));
  }

  for (int i = 1; i < (int)node->children.size(); ++i)
    HandleExpression(ConvertToExpressionNodePtr(node->children[std::size_t(i)]));

  if (f->kind == Function::Kind::UserFunction)
  {
    Script::Instruction instruction(Opcode::InvokeUserFunction, node->token.line);
    instruction.index       = f->index;
    instruction.variant.i32 = num_parameters;
    instruction.type_id     = node->type->id;
    function_->AddInstruction(instruction);
  }
  else
  {
    // Opcode-based free function
    // Opcode-based type constructor
    // Opcode-based type function
    // Opcode-based instance function
    Script::Instruction instruction(f->opcode, node->token.line);
    instruction.variant.i32 = num_parameters;
    instruction.type_id     = node->type->id;
    function_->AddInstruction(instruction);
  }
}

void Generator::ScopeEnter() { scopes_.push_back(Scope()); }

void Generator::ScopeLeave(BlockNodePtr block_node)
{
  const int scope_number = (int)scopes_.size() - 1;
  Scope &   scope        = scopes_[std::size_t(scope_number)];
  if (scope.objects.size() > 0)
  {
    // Note: the function definition block does not require a Destruct instruction
    // because the Return and ReturnValue instructions, which must always be
    // present, already take care of destructing objects
    if (block_node->kind != Node::Kind::FunctionDefinitionStatement)
    {
      // 0 is not correct line number, should be end of block!
      Script::Instruction instruction(Opcode::Destruct, 0);
      // Arrange for all live objects with scope >= scope_number to be destucted
      instruction.variant.i32 = scope_number;
      function_->AddInstruction(instruction);
    }
  }
  scopes_.pop_back();
}

}  // namespace vm
}  // namespace fetch
