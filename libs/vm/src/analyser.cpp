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

#include "vm/analyser.hpp"
#include <sstream>

namespace fetch {
namespace vm {

std::string Analyser::CONSTRUCTOR = "$constructor$";

void Analyser::Initialise()
{
  global_symbol_table_        = CreateSymbolTable();
  next_instantiation_type_id_ = 2000;

  operator_table_ = {{Operator::Equal, Node::Kind::EqualOp},
                     {Operator::NotEqual, Node::Kind::NotEqualOp},
                     {Operator::LessThan, Node::Kind::LessThanOp},
                     {Operator::LessThanOrEqual, Node::Kind::LessThanOrEqualOp},
                     {Operator::GreaterThan, Node::Kind::GreaterThanOp},
                     {Operator::GreaterThanOrEqual, Node::Kind::GreaterThanOrEqualOp},
                     {Operator::UnaryMinus, Node::Kind::UnaryMinusOp},
                     {Operator::Add, Node::Kind::AddOp},
                     {Operator::Subtract, Node::Kind::SubtractOp},
                     {Operator::Multiply, Node::Kind::MultiplyOp},
                     {Operator::Divide, Node::Kind::DivideOp},
                     {Operator::AddAssign, Node::Kind::AddAssignOp},
                     {Operator::SubtractAssign, Node::Kind::SubtractAssignOp},
                     {Operator::MultiplyAssign, Node::Kind::MultiplyAssignOp},
                     {Operator::DivideAssign, Node::Kind::DivideAssignOp}};

  CreateMetaType("Any", TypeIds::Any, any_type_);
  CreateMetaType("TemplateParameter1", TypeIds::TemplateParameter1, template_parameter1_type_);
  CreateMetaType("TemplateParameter2", TypeIds::TemplateParameter2, template_parameter2_type_);

  CreatePrimitiveType("Void", TypeIds::Void, false, void_type_);
  CreatePrimitiveType("Null", TypeIds::Null, false, null_type_);
  CreatePrimitiveType("Bool", TypeIds::Bool, true, bool_type_);
  CreatePrimitiveType("Int8", TypeIds::Int8, true, int8_type_);
  CreatePrimitiveType("Byte", TypeIds::Byte, true, byte_type_);
  CreatePrimitiveType("Int16", TypeIds::Int16, true, int16_type_);
  CreatePrimitiveType("UInt16", TypeIds::UInt16, true, uint16_type_);
  CreatePrimitiveType("Int32", TypeIds::Int32, true, int32_type_);
  CreatePrimitiveType("UInt32", TypeIds::UInt32, true, uint32_type_);
  CreatePrimitiveType("Int64", TypeIds::Int64, true, int64_type_);
  CreatePrimitiveType("UInt64", TypeIds::UInt64, true, uint64_type_);
  CreatePrimitiveType("Float32", TypeIds::Float32, true, float32_type_);
  CreatePrimitiveType("Float64", TypeIds::Float64, true, float64_type_);

  CreateVariantType("IntegerVariant", TypeIds::IntegerVariant,
                    {int8_type_, byte_type_, int16_type_, uint16_type_, int32_type_, uint32_type_,
                     int64_type_, uint64_type_},
                    integer_variant_type_);
  CreateVariantType("RealVariant", TypeIds::RealVariant, {float32_type_, float64_type_},
                    real_variant_type_);
  CreateVariantType("NumberVariant", TypeIds::NumberVariant,
                    {int8_type_, byte_type_, int16_type_, uint16_type_, int32_type_, uint32_type_,
                     int64_type_, uint64_type_, float32_type_, float64_type_},
                    number_variant_type_);
  CreateVariantType("CastVariant", TypeIds::CastVariant,
                    {bool_type_, int8_type_, byte_type_, int16_type_, uint16_type_, int32_type_,
                     uint32_type_, int64_type_, uint64_type_, float32_type_, float64_type_},
                    cast_variant_type_);

  CreateOpcodeFreeFunction("toInt8", Opcodes::ToInt8, {cast_variant_type_}, int8_type_);
  CreateOpcodeFreeFunction("toByte", Opcodes::ToByte, {cast_variant_type_}, byte_type_);
  CreateOpcodeFreeFunction("toInt16", Opcodes::ToInt16, {cast_variant_type_}, int16_type_);
  CreateOpcodeFreeFunction("toUInt16", Opcodes::ToUInt16, {cast_variant_type_}, uint16_type_);
  CreateOpcodeFreeFunction("toInt32", Opcodes::ToInt32, {cast_variant_type_}, int32_type_);
  CreateOpcodeFreeFunction("toUInt32", Opcodes::ToUInt32, {cast_variant_type_}, uint32_type_);
  CreateOpcodeFreeFunction("toInt64", Opcodes::ToInt64, {cast_variant_type_}, int64_type_);
  CreateOpcodeFreeFunction("toUInt64", Opcodes::ToUInt64, {cast_variant_type_}, uint64_type_);
  CreateOpcodeFreeFunction("toFloat32", Opcodes::ToFloat32, {cast_variant_type_}, float32_type_);
  CreateOpcodeFreeFunction("toFloat64", Opcodes::ToFloat64, {cast_variant_type_}, float64_type_);

  CreateClassType("String", TypeIds::String, string_type_);
  EnableOperator(string_type_, Operator::Equal);
  EnableOperator(string_type_, Operator::NotEqual);
  EnableOperator(string_type_, Operator::LessThan);
  EnableOperator(string_type_, Operator::LessThanOrEqual);
  EnableOperator(string_type_, Operator::GreaterThan);
  EnableOperator(string_type_, Operator::GreaterThanOrEqual);
  EnableOperator(string_type_, Operator::Add);

  CreateTemplateType("Matrix", TypeIds::IMatrix, {real_variant_type_}, matrix_type_);
  EnableIndexOperator(matrix_type_, {integer_variant_type_, integer_variant_type_},
                      template_parameter1_type_);
  EnableOperator(matrix_type_, Operator::UnaryMinus);
  EnableOperator(matrix_type_, Operator::Add);
  EnableOperator(matrix_type_, Operator::Subtract);
  EnableOperator(matrix_type_, Operator::Multiply);
  EnableOperator(matrix_type_, Operator::AddAssign);
  EnableOperator(matrix_type_, Operator::SubtractAssign);
  EnableOperator(matrix_type_, Operator::MultiplyAssign);
  EnableLeftOperator(matrix_type_, Operator::Multiply);
  EnableRightOperator(matrix_type_, Operator::Add);
  EnableRightOperator(matrix_type_, Operator::Subtract);
  EnableRightOperator(matrix_type_, Operator::Multiply);
  EnableRightOperator(matrix_type_, Operator::Divide);
  EnableRightOperator(matrix_type_, Operator::AddAssign);
  EnableRightOperator(matrix_type_, Operator::SubtractAssign);
  EnableRightOperator(matrix_type_, Operator::MultiplyAssign);
  EnableRightOperator(matrix_type_, Operator::DivideAssign);

  CreateTemplateType("Array", TypeIds::IArray, {any_type_}, array_type_);
  EnableIndexOperator(array_type_, {integer_variant_type_}, template_parameter1_type_);

  CreateTemplateType("Map", TypeIds::IMap, {any_type_, any_type_}, map_type_);
  EnableIndexOperator(map_type_, {template_parameter1_type_}, template_parameter2_type_);

  // ledger specific
  CreateClassType("Address", TypeIds::Address, address_type_);
  EnableOperator(address_type_, Operator::Equal);
  EnableOperator(address_type_, Operator::NotEqual);

  CreateTemplateType("State", TypeIds::IState, {any_type_}, state_type_);
}

void Analyser::UnInitialise()
{
  for (auto const &type : types_)
  {
    type->Reset();
  }
  types_.clear();
  type_table_.clear();
  type_info_table_.clear();
  if (global_symbol_table_)
  {
    global_symbol_table_->Reset();
    global_symbol_table_ = nullptr;
  }
  any_type_                 = nullptr;
  template_parameter1_type_ = nullptr;
  template_parameter2_type_ = nullptr;
  void_type_                = nullptr;
  null_type_                = nullptr;
  bool_type_                = nullptr;
  int8_type_                = nullptr;
  byte_type_                = nullptr;
  int16_type_               = nullptr;
  uint16_type_              = nullptr;
  int32_type_               = nullptr;
  uint32_type_              = nullptr;
  int64_type_               = nullptr;
  uint64_type_              = nullptr;
  float32_type_             = nullptr;
  float64_type_             = nullptr;
  integer_variant_type_     = nullptr;
  real_variant_type_        = nullptr;
  number_variant_type_      = nullptr;
  cast_variant_type_        = nullptr;
  matrix_type_              = nullptr;
  array_type_               = nullptr;
  map_type_                 = nullptr;
  state_type_               = nullptr;
  string_type_              = nullptr;
  op_table_.clear();
  left_op_table_.clear();
  right_op_table_.clear();
}

void Analyser::CreateClassType(std::string const &name, TypeId id)
{
  TypePtr type;
  CreateClassType(name, id, type);
}

void Analyser::CreateTemplateInstantiationType(TypeId id, TypeId template_type_id,
                                               TypeIdArray const &parameter_type_ids)
{
  TypePtr type;
  InternalCreateTemplateInstantiationType(
      id, GetTypePtr(template_type_id), GetTypePtrs(parameter_type_ids), parameter_type_ids, type);
}

void Analyser::CreateOpcodeFreeFunction(std::string const &name, Opcode opcode,
                                        TypeIdArray const &parameter_type_ids,
                                        TypeId             return_type_id)
{
  CreateOpcodeFreeFunction(name, opcode, GetTypePtrs(parameter_type_ids),
                           GetTypePtr(return_type_id));
}

void Analyser::CreateOpcodeTypeConstructor(TypeId type_id, Opcode opcode,
                                           TypeIdArray const &parameter_type_ids)
{
  CreateOpcodeTypeConstructor(GetTypePtr(type_id), opcode, GetTypePtrs(parameter_type_ids));
}

void Analyser::CreateOpcodeTypeFunction(TypeId type_id, std::string const &name, Opcode opcode,
                                        TypeIdArray const &parameter_type_ids,
                                        TypeId             return_type_id)
{
  CreateOpcodeTypeFunction(GetTypePtr(type_id), name, opcode, GetTypePtrs(parameter_type_ids),
                           GetTypePtr(return_type_id));
}

void Analyser::CreateOpcodeInstanceFunction(TypeId type_id, std::string const &name, Opcode opcode,
                                            TypeIdArray const &parameter_type_ids,
                                            TypeId             return_type_id)
{
  CreateOpcodeInstanceFunction(GetTypePtr(type_id), name, opcode, GetTypePtrs(parameter_type_ids),
                               GetTypePtr(return_type_id));
}

void Analyser::EnableOperator(TypeId type_id, Operator op)
{
  EnableOperator(GetTypePtr(type_id), op);
}

void Analyser::EnableIndexOperator(TypeId type_id, TypeIdArray const &input_type_ids,
                                   TypeId const &output_type_id)
{
  EnableIndexOperator(GetTypePtr(type_id), GetTypePtrs(input_type_ids), GetTypePtr(output_type_id));
}

bool Analyser::Analyse(BlockNodePtr const &root, TypeInfoTable &type_info_table, Strings &errors)
{
  root_ = root;
  blocks_.clear();
  loops_.clear();
  function_ = nullptr;
  errors_.clear();

  root_->symbol_table = CreateSymbolTable();

  // Create symbol tables for all blocks
  // Check function prototypes
  BuildBlock(root_);

  if (errors_.size() != 0)
  {
    type_info_table.clear();
    errors = std::move(errors_);
    root_  = nullptr;
    blocks_.clear();
    loops_.clear();
    function_ = nullptr;
    errors_.clear();
    return false;
  }

  AnnotateBlock(root_);

  root_ = nullptr;
  blocks_.clear();
  loops_.clear();
  function_ = nullptr;

  if (errors_.size() != 0)
  {
    type_info_table.clear();
    errors = std::move(errors_);
    errors_.clear();
    return false;
  }

  type_info_table = type_info_table_;
  errors.clear();
  return true;
}

void Analyser::AddError(Token const &token, std::string const &message)
{
  std::stringstream stream;
  stream << "line " << token.line << ": "
         << "error: " << message;
  errors_.push_back(stream.str());
}

void Analyser::BuildBlock(BlockNodePtr const &block_node)
{
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->kind)
    {
    case Node::Kind::FunctionDefinitionStatement:
    {
      BuildFunctionDefinition(block_node, ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::WhileStatement:
    {
      BuildWhileStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::ForStatement:
    {
      BuildForStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::IfStatement:
    {
      BuildIfStatement(child);
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }
}

void Analyser::BuildFunctionDefinition(BlockNodePtr const &parent_block_node,
                                       BlockNodePtr const &function_definition_node)
{
  function_definition_node->symbol_table = CreateSymbolTable();
  ExpressionNodePtr identifier_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  std::string const &name  = identifier_node->token.text;
  int const          count = (int)function_definition_node->children.size();
  VariablePtrArray   parameter_variables;
  TypePtrArray       parameter_types;
  int const          num_parameters = int((count - 3) / 2);
  int                problems       = 0;
  for (int i = 0; i < num_parameters; ++i)
  {
    ExpressionNodePtr parameter_node =
        ConvertToExpressionNodePtr(function_definition_node->children[size_t(2 + i * 2)]);
    std::string const &parameter_name = parameter_node->token.text;
    SymbolPtr          symbol = function_definition_node->symbol_table->Find(parameter_name);
    if (symbol)
    {
      std::stringstream stream;
      AddError(parameter_node->token, "parameter name '" + parameter_name + "' is already defined");
      ++problems;
      continue;
    }
    ExpressionNodePtr parameter_type_node =
        ConvertToExpressionNodePtr(function_definition_node->children[size_t(3 + i * 2)]);
    TypePtr parameter_type = FindType(parameter_type_node);
    if (parameter_type == nullptr)
    {
      AddError(parameter_type_node->token,
               "unknown type '" + parameter_type_node->token.text + "'");
      ++problems;
      continue;
    }
    VariablePtr parameter_variable = CreateVariable(parameter_name, Variable::Category::Parameter);
    parameter_variable->type       = parameter_type;
    function_definition_node->symbol_table->Add(parameter_name, parameter_variable);
    parameter_node->variable = parameter_variable;
    parameter_node->type     = parameter_variable->type;
    parameter_variables.push_back(parameter_variable);
    parameter_types.push_back(parameter_type);
  }
  TypePtr           return_type;
  ExpressionNodePtr return_type_node =
      ConvertToExpressionNodePtr(function_definition_node->children[size_t(count - 1)]);
  if (return_type_node)
  {
    return_type = FindType(return_type_node);
    if (return_type == nullptr)
    {
      AddError(return_type_node->token, "unknown type '" + return_type_node->token.text + "'");
      ++problems;
    }
  }
  else
  {
    return_type = void_type_;
  }
  if (problems)
  {
    return;
  }
  FunctionGroupPtr fg;
  SymbolPtr        symbol = parent_block_node->symbol_table->Find(name);
  if (symbol)
  {
    fg = ConvertToFunctionGroupPtr(symbol);
    TypePtrArray dummy;
    if (FindFunction(nullptr, fg, parameter_types, dummy))
    {
      AddError(function_definition_node->token,
               "function '" + name + "' is already defined with the same parameter types");
      return;
    }
  }
  else
  {
    fg = CreateFunctionGroup(name);
    parent_block_node->symbol_table->Add(name, fg);
  }
  FunctionPtr function =
      CreateUserFunction(name, parameter_types, parameter_variables, return_type);
  fg->functions.push_back(function);
  identifier_node->function = function;
  BuildBlock(function_definition_node);
}

void Analyser::BuildWhileStatement(BlockNodePtr const &while_statement_node)
{
  while_statement_node->symbol_table = CreateSymbolTable();
  BuildBlock(while_statement_node);
}

void Analyser::BuildForStatement(BlockNodePtr const &for_statement_node)
{
  for_statement_node->symbol_table = CreateSymbolTable();
  BuildBlock(for_statement_node);
}

void Analyser::BuildIfStatement(NodePtr const &if_statement_node)
{
  for (NodePtr const &child : if_statement_node->children)
  {
    BlockNodePtr block_node  = ConvertToBlockNodePtr(child);
    block_node->symbol_table = CreateSymbolTable();
    BuildBlock(block_node);
  }
}

void Analyser::AnnotateBlock(BlockNodePtr const &block_node)
{
  blocks_.push_back(block_node);
  bool const is_loop = ((block_node->kind == Node::Kind::WhileStatement) ||
                        (block_node->kind == Node::Kind::ForStatement));
  if (is_loop)
  {
    loops_.push_back(block_node);
  }
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->kind)
    {
    case Node::Kind::FunctionDefinitionStatement:
    {
      AnnotateFunctionDefinitionStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::WhileStatement:
    {
      AnnotateWhileStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::ForStatement:
    {
      AnnotateForStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::IfStatement:
    {
      AnnotateIfStatement(child);
      break;
    }
    case Node::Kind::VarDeclarationStatement:
    case Node::Kind::VarDeclarationTypedAssignmentStatement:
    case Node::Kind::VarDeclarationTypelessAssignmentStatement:
    {
      AnnotateVarStatement(block_node, child);
      break;
    }
    case Node::Kind::ReturnStatement:
    {
      AnnotateReturnStatement(child);
      break;
    }
    case Node::Kind::BreakStatement:
    {
      if (loops_.size() == 0)
      {
        AddError(child->token, "break statement is not inside a while or for loop");
      }
      break;
    }
    case Node::Kind::ContinueStatement:
    {
      if (loops_.size() == 0)
      {
        AddError(child->token, "continue statement is not inside a while or for loop");
      }
      break;
    }
    case Node::Kind::AssignOp:
    {
      AnnotateAssignOp(ConvertToExpressionNodePtr(child));
      break;
    }
    case Node::Kind::ModuloAssignOp:
    {
      AnnotateModuloAssignOp(ConvertToExpressionNodePtr(child));
      break;
    }
    case Node::Kind::AddAssignOp:
    case Node::Kind::SubtractAssignOp:
    case Node::Kind::MultiplyAssignOp:
    case Node::Kind::DivideAssignOp:
    {
      AnnotateArithmeticAssignOp(ConvertToExpressionNodePtr(child));
      break;
    }
    default:
    {
      AnnotateExpression(ConvertToExpressionNodePtr(child));
      break;
    }
    }  // switch
  }
  if (is_loop)
  {
    loops_.pop_back();
  }
  blocks_.pop_back();
}

void Analyser::AnnotateFunctionDefinitionStatement(BlockNodePtr const &function_definition_node)
{
  ExpressionNodePtr identifier_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  function_ = identifier_node->function;
  AnnotateBlock(function_definition_node);
  if (errors_.size() == 0)
  {
    if (function_->return_type->id != TypeIds::Void)
    {
      if (TestBlock(function_definition_node))
      {
        AddError(identifier_node->token,
                 "control reaches end of function without returning a value");
      }
    }
  }
  function_ = nullptr;
}

void Analyser::AnnotateWhileStatement(BlockNodePtr const &while_statement_node)
{
  AnnotateConditionalBlock(while_statement_node);
}

void Analyser::AnnotateForStatement(BlockNodePtr const &for_statement_node)
{
  ExpressionNodePtr  identifier_node = ConvertToExpressionNodePtr(for_statement_node->children[0]);
  std::string const &name            = identifier_node->token.text;
  // Note: variable is created with no type
  VariablePtr variable = CreateVariable(name, Variable::Category::For);
  for_statement_node->symbol_table->Add(name, variable);
  identifier_node->variable            = variable;
  size_t const                   count = for_statement_node->children.size() - 1;
  std::vector<ExpressionNodePtr> nodes;
  int                            problems = 0;
  for (size_t i = 1; i <= count; ++i)
  {
    NodePtr const &   child      = for_statement_node->children[i];
    ExpressionNodePtr child_node = ConvertToExpressionNodePtr(child);
    if (!AnnotateExpression(child_node))
    {
      ++problems;
      continue;
    }
    if (!IsIntegerType(child_node->type))
    {
      ++problems;
      AddError(child_node->token, "integral type expected");
      continue;
    }
    nodes.push_back(child_node);
  }
  if (problems == 0)
  {
    if (nodes[0]->type == nodes[1]->type)
    {
      if ((count == 3) && (nodes[1]->type != nodes[2]->type))
      {
        AddError(nodes[1]->token, "incompatible types");
        ++problems;
      }
    }
    else
    {
      AddError(nodes[0]->token, "incompatible types");
      ++problems;
    }
  }
  if (problems == 0)
  {
    TypePtr inferred_for_variable_type = nodes[0]->type;
    identifier_node->variable->type    = inferred_for_variable_type;
  }
  AnnotateBlock(for_statement_node);
}

void Analyser::AnnotateIfStatement(NodePtr const &if_statement_node)
{
  for (NodePtr const &child : if_statement_node->children)
  {
    BlockNodePtr child_block_node = ConvertToBlockNodePtr(child);
    if (child_block_node->kind != Node::Kind::Else)
    {
      AnnotateConditionalBlock(child_block_node);
    }
    else
    {
      AnnotateBlock(child_block_node);
    }
  }
}

void Analyser::AnnotateVarStatement(BlockNodePtr const &parent_block_node,
                                    NodePtr const &     var_statement_node)
{
  ExpressionNodePtr  identifier_node = ConvertToExpressionNodePtr(var_statement_node->children[0]);
  std::string const &name            = identifier_node->token.text;
  SymbolPtr          symbol          = parent_block_node->symbol_table->Find(name);
  if (symbol)
  {
    AddError(identifier_node->token, "variable '" + name + "' is already defined");
    return;
  }
  // Note: variable is created with no type
  VariablePtr variable = CreateVariable(name, Variable::Category::Local);
  parent_block_node->symbol_table->Add(name, variable);
  identifier_node->variable = variable;
  if (var_statement_node->kind == Node::Kind::VarDeclarationStatement)
  {
    ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (!AnnotateTypeExpression(type_node))
    {
      return;
    }
    variable->type = type_node->type;
  }
  else if (var_statement_node->kind == Node::Kind::VarDeclarationTypedAssignmentStatement)
  {
    ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (!AnnotateTypeExpression(type_node))
    {
      return;
    }
    ExpressionNodePtr expression_node = ConvertToExpressionNodePtr(var_statement_node->children[2]);
    if (!AnnotateExpression(expression_node))
    {
      return;
    }
    if (expression_node->type->id != TypeIds::Null)
    {
      if (type_node->type != expression_node->type)
      {
        AddError(type_node->token, "incompatible types");
        return;
      }
    }
    else
    {
      if (type_node->type->category == TypeCategory::Primitive)
      {
        // Can't assign null to a primitive type
        AddError(type_node->token, "unable to assign null to primitive type");
        return;
      }
      // Convert the null type to the declared type
      expression_node->type = type_node->type;
    }
    variable->type = type_node->type;
  }
  else
  {
    ExpressionNodePtr expression_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (!AnnotateExpression(expression_node))
    {
      return;
    }
    if ((expression_node->type->id == TypeIds::Void) ||
        (expression_node->type->id == TypeIds::Null))
    {
      AddError(expression_node->token, "unable to infer type");
      return;
    }
    variable->type = expression_node->type;
  }
}

void Analyser::AnnotateReturnStatement(NodePtr const &return_statement_node)
{
  if (return_statement_node->children.size() == 1)
  {
    ExpressionNodePtr expression_node =
        ConvertToExpressionNodePtr(return_statement_node->children[0]);
    if (!AnnotateExpression(expression_node))
    {
      return;
    }
    if (expression_node->type->id != TypeIds::Null)
    {
      // note: function_->return_type can be Void
      if (expression_node->type != function_->return_type)
      {
        AddError(expression_node->token, "type does not match function return type");
        return;
      }
    }
    else
    {
      // note: function_->return_type can be Void
      if (function_->return_type->category == TypeCategory::Primitive)
      {
        AddError(expression_node->token, "unable to return null");
        return;
      }
      // Convert the null type to the known return type of the function
      expression_node->type = function_->return_type;
    }
  }
  else
  {
    if (function_->return_type->id != TypeIds::Void)
    {
      AddError(return_statement_node->token, "return does not supply a value");
      return;
    }
  }
}

void Analyser::AnnotateConditionalBlock(BlockNodePtr const &conditional_node)
{
  ExpressionNodePtr expression_node = ConvertToExpressionNodePtr(conditional_node->children[0]);
  if (AnnotateExpression(expression_node))
  {
    if (expression_node->type->id != TypeIds::Bool)
    {
      AddError(expression_node->token, "boolean type expected");
    }
  }
  AnnotateBlock(conditional_node);
}

bool Analyser::AnnotateTypeExpression(ExpressionNodePtr const &node)
{
  TypePtr type = FindType(node);
  if (type == nullptr)
  {
    AddError(node->token, "unknown type '" + node->token.text + "'");
    return false;
  }
  SetType(node, type);
  return true;
}

bool Analyser::AnnotateAssignOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (!IsWriteable(lhs))
  {
    return false;
  }
  if (rhs->type->id == TypeIds::Void)
  {
    // Can't assign from a function with no return value
    AddError(node->token, "incompatible types");
    return false;
  }
  if (rhs->type->id != TypeIds::Null)
  {
    if (lhs->type != rhs->type)
    {
      AddError(node->token, "incompatible types");
      return false;
    }
  }
  else
  {
    if (lhs->type->category == TypeCategory::Primitive)
    {
      // Can't assign null to a primitive type
      AddError(node->token, "incompatible types");
      return false;
    }
    // Convert the null type to the correct type
    rhs->type = lhs->type;
  }
  SetRV(node, lhs->type);
  return true;
}

bool Analyser::AnnotateModuloAssignOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (!IsWriteable(lhs))
  {
    return false;
  }
  if ((lhs->type == rhs->type) && IsIntegerType(lhs->type))
  {
    SetRV(node, lhs->type);
    return true;
  }
  AddError(node->token, "integral type expected");
  return false;
}

bool Analyser::AnnotateArithmeticAssignOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (!IsWriteable(lhs))
  {
    return false;
  }
  TypePtr type = IsCompatible(node, lhs, rhs);
  if (type == nullptr)
  {
    return false;
  }
  SetRV(node, type);
  return true;
}

bool Analyser::AnnotateExpression(ExpressionNodePtr const &node)
{
  switch (node->kind)
  {
  case Node::Kind::Identifier:
  case Node::Kind::Template:
  {
    SymbolPtr symbol = FindSymbol(node);
    if (symbol == nullptr)
    {
      AddError(node->token, "unknown symbol '" + node->token.text + "'");
      return false;
    }
    if (!symbol->IsVariable())
    {
      // type or a function name by itself
      AddError(node->token, "symbol '" + node->token.text + "' is not a variable");
      return false;
    }
    VariablePtr variable = ConvertToVariablePtr(symbol);
    if (variable->type == nullptr)
    {
      AddError(node->token, "variable '" + node->token.text + "' has unresolved type");
      return false;
    }
    SetVariable(node, variable);
    break;
  }
  case Node::Kind::Integer32:
  {
    SetRV(node, int32_type_);
    break;
  }
  case Node::Kind::UnsignedInteger32:
  {
    SetRV(node, uint32_type_);
    break;
  }
  case Node::Kind::Integer64:
  {
    SetRV(node, int64_type_);
    break;
  }
  case Node::Kind::UnsignedInteger64:
  {
    SetRV(node, uint64_type_);
    break;
  }
  case Node::Kind::Float32:
  {
    SetRV(node, float32_type_);
    break;
  }
  case Node::Kind::Float64:
  {
    SetRV(node, float64_type_);
    break;
  }
  case Node::Kind::String:
  {
    SetRV(node, string_type_);
    break;
  }
  case Node::Kind::True:
  case Node::Kind::False:
  {
    SetRV(node, bool_type_);
    break;
  }
  case Node::Kind::Null:
  {
    SetRV(node, null_type_);
    break;
  }
  case Node::Kind::EqualOp:
  case Node::Kind::NotEqualOp:
  {
    if (!AnnotateEqualityOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::LessThanOp:
  case Node::Kind::LessThanOrEqualOp:
  case Node::Kind::GreaterThanOp:
  case Node::Kind::GreaterThanOrEqualOp:
  {
    if (!AnnotateRelationalOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::AndOp:
  case Node::Kind::OrOp:
  {
    if (!AnnotateBinaryLogicalOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::NotOp:
  {
    if (!AnnotateUnaryLogicalOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::PrefixIncOp:
  case Node::Kind::PrefixDecOp:
  case Node::Kind::PostfixIncOp:
  case Node::Kind::PostfixDecOp:
  {
    if (!AnnotateIncDecOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::UnaryMinusOp:
  {
    if (!AnnotateUnaryMinusOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::ModuloOp:
  {
    if (!AnnotateModuloOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::AddOp:
  case Node::Kind::SubtractOp:
  case Node::Kind::MultiplyOp:
  case Node::Kind::DivideOp:
  {
    if (!AnnotateArithmeticOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::IndexOp:
  {
    if (!AnnotateIndexOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::DotOp:
  {
    if (!AnnotateDotOp(node))
    {
      return false;
    }
    break;
  }
  case Node::Kind::InvokeOp:
  {
    if (!AnnotateInvokeOp(node))
    {
      return false;
    }
    break;
  }
  default:
  {
    AddError(node->token, "internal error at '" + node->token.text + "'");
    return false;
  }
  }  // switch
  return true;
}

bool Analyser::AnnotateEqualityOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if ((lhs->type->id == TypeIds::Void) || (rhs->type->id == TypeIds::Void))
  {
    AddError(node->token, "unable to compare operand(s) of type Void");
    return false;
  }
  bool const lhs_is_concrete_type = lhs->type->id != TypeIds::Null;
  bool const rhs_is_concrete_type = rhs->type->id != TypeIds::Null;
  bool const lhs_is_primitive     = lhs->type->category == TypeCategory::Primitive;
  bool const rhs_is_primitive     = rhs->type->category == TypeCategory::Primitive;
  if (lhs_is_concrete_type)
  {
    if (rhs_is_concrete_type)
    {
      if (lhs->type != rhs->type)
      {
        AddError(node->token, "incompatible types");
        return false;
      }
      Node::Kind const op      = node->kind;
      bool const       enabled = lhs_is_primitive || IsOpEnabled(lhs->type, op);
      if (!enabled)
      {
        AddError(node->token, "operator not supported");
        return false;
      }
    }
    else
    {
      if (lhs_is_primitive)
      {
        // unable to compare LHS primitive type to RHS null
        AddError(node->token, "incompatible types");
        return false;
      }
      // Convert the RHS null type to the correct type
      rhs->type = lhs->type;
    }
  }
  else
  {
    if (rhs_is_concrete_type)
    {
      if (rhs_is_primitive)
      {
        // unable to compare LHS null to RHS primitive type
        AddError(node->token, "incompatible types");
        return false;
      }
      // Convert the LHS null type to the correct type
      lhs->type = rhs->type;
    }
    else
    {
      // Comparing null to null -- this is allowed
    }
  }
  SetRV(node, bool_type_);
  return true;
}

bool Analyser::AnnotateRelationalOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (lhs->type != rhs->type)
  {
    AddError(node->token, "incompatible types");
    return false;
  }
  Node::Kind const op      = node->kind;
  bool const       enabled = IsNumberType(lhs->type) || IsOpEnabled(lhs->type, op);
  if (!enabled)
  {
    AddError(node->token, "operator not supported");
    return false;
  }
  SetRV(node, bool_type_);
  return true;
}

bool Analyser::AnnotateBinaryLogicalOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if ((lhs->type->id != TypeIds::Bool) || (rhs->type->id != TypeIds::Bool))
  {
    AddError(node->token, "boolean operands expected");
    return false;
  }
  SetRV(node, bool_type_);
  return true;
}

bool Analyser::AnnotateUnaryLogicalOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (!AnnotateExpression(operand))
  {
    return false;
  }
  if (operand->type->id != TypeIds::Bool)
  {
    AddError(node->token, "boolean operand expected");
    return false;
  }
  SetRV(node, bool_type_);
  return true;
}

bool Analyser::AnnotateIncDecOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (!AnnotateExpression(operand))
  {
    return false;
  }
  if (!IsWriteable(operand))
  {
    return false;
  }
  if (!IsIntegerType(operand->type))
  {
    AddError(node->token, "integral type expected");
    return false;
  }
  SetRV(node, operand->type);
  return true;
}

bool Analyser::AnnotateUnaryMinusOp(ExpressionNodePtr const &node)
{
  Node::Kind const  op      = node->kind;
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (!AnnotateExpression(operand))
  {
    return false;
  }
  if (IsNumberType(operand->type) || IsOpEnabled(operand->type, op))
  {
    SetRV(node, operand->type);
    return true;
  }
  AddError(node->token, "operator not supported");
  return false;
}

bool Analyser::AnnotateModuloOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if ((lhs->type == rhs->type) && IsIntegerType(lhs->type))
  {
    SetRV(node, lhs->type);
    return true;
  }
  AddError(node->token, "integral type expected");
  return false;
}

bool Analyser::AnnotateArithmeticOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs  = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs  = ConvertToExpressionNodePtr(node->children[1]);
  TypePtr           type = IsCompatible(node, lhs, rhs);
  if (type == nullptr)
  {
    return false;
  }
  SetRV(node, type);
  return true;
}

bool Analyser::AnnotateIndexOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  if ((lhs->kind == Node::Kind::Identifier) || (lhs->kind == Node::Kind::Template))
  {
    SymbolPtr symbol = FindSymbol(lhs);
    if (symbol == nullptr)
    {
      AddError(lhs->token, "unknown symbol '" + lhs->token.text + "'");
      return false;
    }
    if (!symbol->IsVariable())
    {
      AddError(lhs->token, "operand does not support index operator");
      return false;
    }
    VariablePtr variable = ConvertToVariablePtr(symbol);
    if (variable->type == nullptr)
    {
      AddError(lhs->token, "variable '" + lhs->token.text + "' has unresolved type");
      return false;
    }
    SetVariable(lhs, variable);
  }
  else
  {
    if (!AnnotateExpression(lhs))
    {
      return false;
    }
  }

  if ((lhs->category == ExpressionNode::Category::Type) ||
      (lhs->category == ExpressionNode::Category::Function))
  {
    AddError(lhs->token, "operand does not support index operator");
    return false;
  }

  TypePtr type;
  if (lhs->type->category == TypeCategory::TemplateInstantiation)
  {
    type = lhs->type->template_type;
  }
  else
  {
    type = lhs->type;
  }

  size_t const num_expected_indexes = type->index_input_types.size();
  if (num_expected_indexes == 0)
  {
    AddError(lhs->token, "operand does not support index operator");
    return false;
  }

  size_t const num_supplied_indexes = node->children.size() - 1;
  if (num_supplied_indexes != num_expected_indexes)
  {
    AddError(lhs->token, "incorrect number of operands for index operator");
    return false;
  }

  TypePtrArray supplied_index_types;
  for (size_t i = 1; i <= num_supplied_indexes; ++i)
  {
    NodePtr const &   supplied_index      = node->children[i];
    ExpressionNodePtr supplied_index_node = ConvertToExpressionNodePtr(supplied_index);
    if (!AnnotateExpression(supplied_index_node))
    {
      return false;
    }
    supplied_index_types.push_back(supplied_index_node->type);
  }

  TypePtrArray actual_index_types;
  bool const   match =
      MatchTypes(lhs->type, supplied_index_types, type->index_input_types, actual_index_types);
  if (!match)
  {
    AddError(lhs->token, "incorrect operands for index operator");
    return false;
  }

  for (size_t i = 1; i <= num_supplied_indexes; ++i)
  {
    NodePtr const &   supplied_index      = node->children[i];
    ExpressionNodePtr supplied_index_node = ConvertToExpressionNodePtr(supplied_index);
    supplied_index_node->type             = actual_index_types[i - 1];
  }

  TypePtr result_type = ConvertType(type->index_output_type, lhs->type);
  SetLV(node, result_type);
  return true;
}

bool Analyser::AnnotateDotOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr  lhs         = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr  rhs         = ConvertToExpressionNodePtr(node->children[1]);
  std::string const &member_name = rhs->token.text;
  if ((lhs->kind == Node::Kind::Identifier) || (lhs->kind == Node::Kind::Template))
  {
    SymbolPtr symbol = FindSymbol(lhs);
    if (symbol == nullptr)
    {
      AddError(lhs->token, "unknown symbol '" + lhs->token.text + "'");
      return false;
    }
    if (symbol->IsVariable())
    {
      VariablePtr variable = ConvertToVariablePtr(symbol);
      if (variable->type == nullptr)
      {
        AddError(lhs->token, "variable '" + lhs->token.text + "' has unresolved type");
        return false;
      }
      SetVariable(lhs, variable);
    }
    else if (symbol->IsType())
    {
      TypePtr type = ConvertToTypePtr(symbol);
      SetType(lhs, type);
    }
    else
    {
      // function
      AddError(lhs->token, "operand does not support member-access operator");
      return false;
    }
  }
  else
  {
    if (!AnnotateExpression(lhs))
    {
      return false;
    }
  }

  if (lhs->category == ExpressionNode::Category::Function)
  {
    AddError(lhs->token, "operand does not support member-access operator");
    return false;
  }

  bool const lhs_is_instance = (lhs->category == ExpressionNode::Category::Variable) ||
                               (lhs->category == ExpressionNode::Category::LV) ||
                               (lhs->category == ExpressionNode::Category::RV);

  if (lhs->type->category == TypeCategory::Primitive)
  {
    AddError(lhs->token,
             "primitive type '" + lhs->type->name + "' does not support member-access operator");
    return false;
  }
  SymbolPtr symbol;
  if (lhs->type->category == TypeCategory::TemplateInstantiation)
  {
    symbol = lhs->type->template_type->symbol_table->Find(member_name);
  }
  else
  {
    symbol = lhs->type->symbol_table->Find(member_name);
  }
  if (symbol == nullptr)
  {
    AddError(lhs->token,
             "type '" + lhs->type->name + "' has no member named '" + member_name + "'");
    return false;
  }
  if (symbol->IsFunctionGroup())
  {
    FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
    // Opcode-invoked type constructor  (lhs_is_instance false)
    // Opcode-invoked type function		  (lhs_is_instance false)
    // Opcode-invoked instance function (lhs_is_instance true)
    SetFunction(node, fg, lhs->type, lhs_is_instance);
    return true;
  }
  else if (symbol->IsVariable())
  {
    // type variable or instance variable
    AddError(lhs->token, "not supported");
    return false;
  }
  else
  {
    AddError(lhs->token, "not supported");
    return false;
  }
}

bool Analyser::AnnotateInvokeOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  if ((lhs->kind == Node::Kind::Identifier) || (lhs->kind == Node::Kind::Template))
  {
    SymbolPtr symbol = FindSymbol(lhs);
    if (symbol == nullptr)
    {
      AddError(lhs->token, "unknown symbol '" + lhs->token.text + "'");
      return false;
    }
    if (symbol->IsFunctionGroup())
    {
      FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
      // We've got a user function or an opcode function, type is nullptr for both
      SetFunction(lhs, fg, nullptr, false);
    }
    else if (symbol->IsType())
    {
      // Type constructor
      TypePtr type = ConvertToTypePtr(symbol);
      SetType(lhs, type);
    }
    else
    {
      // Variable
      AddError(lhs->token, "operand does not support function-call operator");
      return false;
    }
  }
  else
  {
    if (!AnnotateExpression(lhs))
    {
      return false;
    }
  }

  TypePtrArray supplied_parameter_types;
  for (size_t i = 1; i < node->children.size(); ++i)
  {
    NodePtr const &   supplied_parameter      = node->children[i];
    ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
    if (!AnnotateExpression(supplied_parameter_node))
    {
      return false;
    }
    supplied_parameter_types.push_back(supplied_parameter_node->type);
  }

  if (lhs->category == ExpressionNode::Category::Function)
  {
    // user function (lhs->type is nullptr)
    // opcode-invoked free function (lhs->type is nullptr)
    // opcode-invoked type function
    // opcode-invoked instance function
    TypePtrArray actual_parameter_types;
    FunctionPtr  f =
        FindFunction(lhs->type, lhs->fg, supplied_parameter_types, actual_parameter_types);
    if (f == nullptr)
    {
      // No matching function, or ambiguous
      AddError(lhs->token, "unable to find matching function for '" + lhs->fg->name + "'");
      return false;
    }

    if (f->kind == Function::Kind::OpcodeTypeFunction)
    {
      if (lhs->function_invoked_on_instance)
      {
        AddError(lhs->token,
                 "function '" + lhs->fg->name + "' is a type function, not an instance function");
        return false;
      }
    }
    else if (f->kind == Function::Kind::OpcodeInstanceFunction)
    {
      if (!lhs->function_invoked_on_instance)
      {
        AddError(lhs->token,
                 "function '" + lhs->fg->name + "' is an instance function, not a type function");
        return false;
      }
    }

    for (size_t i = 1; i < node->children.size(); ++i)
    {
      NodePtr const &   supplied_parameter      = node->children[i];
      ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
      supplied_parameter_node->type             = actual_parameter_types[i - 1];
    }

    TypePtr return_type = ConvertType(f->return_type, lhs->type);
    SetRV(node, return_type);
    node->function = f;
    return true;
  }
  else if (lhs->category == ExpressionNode::Category::Type)
  {
    // Constructor
    if (lhs->type->category == TypeCategory::Primitive)
    {
      AddError(lhs->token, "primitive type '" + lhs->type->name + "' is not constructible");
      return false;
    }
    SymbolPtr symbol;
    if (lhs->type->category == TypeCategory::TemplateInstantiation)
    {
      symbol = lhs->type->template_type->symbol_table->Find(CONSTRUCTOR);
    }
    else
    {
      symbol = lhs->type->symbol_table->Find(CONSTRUCTOR);
    }
    if (symbol == nullptr)
    {
      AddError(lhs->token,
               "unable to find matching constructor for type '" + lhs->type->name + "'");
      return false;
    }

    FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
    TypePtrArray     actual_parameter_types;
    FunctionPtr f = FindFunction(lhs->type, fg, supplied_parameter_types, actual_parameter_types);
    if (f == nullptr)
    {
      // No matching constructor, or ambiguous
      AddError(lhs->token,
               "unable to find matching constructor for type/function '" + lhs->type->name + "'");
      return false;
    }

    for (size_t i = 1; i < node->children.size(); ++i)
    {
      NodePtr const &   supplied_parameter      = node->children[i];
      ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
      supplied_parameter_node->type             = actual_parameter_types[i - 1];
    }

    SetRV(node, lhs->type);
    node->function = f;
    return true;
  }
  else
  {
    // e.g.
    // (a + b)();
    // array[index]();
    AddError(lhs->token, "operand does not support function-call operator");
    return false;
  }
}

// Returns true if control is able to reach the end of the block
bool Analyser::TestBlock(BlockNodePtr const &block_node)
{
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->kind)
    {
    case Node::Kind::ReturnStatement:
    case Node::Kind::BreakStatement:
    case Node::Kind::ContinueStatement:
    {
      return false;
    }
    case Node::Kind::IfStatement:
    {
      bool const got_else = child->children.back()->kind == Node::Kind::Else;
      if (got_else)
      {
        bool able_to_reach_end_of_if_statement = false;
        for (NodePtr const &nodeptr : child->children)
        {
          BlockNodePtr node                             = ConvertToBlockNodePtr(nodeptr);
          bool const   able_to_reach_reach_end_of_block = TestBlock(node);
          able_to_reach_end_of_if_statement |= able_to_reach_reach_end_of_block;
        }
        if (!able_to_reach_end_of_if_statement)
        {
          return false;
        }
      }
      break;
    }
    case Node::Kind::If:
    case Node::Kind::ElseIf:
    case Node::Kind::Else:
    {
      return TestBlock(ConvertToBlockNodePtr(child));
    }
    default:
    {
      break;
    }
    }  // switch
  }
  // No child returned false, so we are indeed able to reach the end of the block
  return true;
}

bool Analyser::IsWriteable(ExpressionNodePtr const &lhs)
{
  if ((lhs->category != ExpressionNode::Category::Variable) &&
      (lhs->category != ExpressionNode::Category::LV))
  {
    AddError(lhs->token, "assignment operand is not writeable");
    return false;
  }
  if (lhs->category == ExpressionNode::Category::Variable)
  {
    if ((lhs->variable->category == Variable::Category::Parameter) ||
        (lhs->variable->category == Variable::Category::For))
    {
      AddError(lhs->token, "assignment operand is not writeable");
      return false;
    }
  }
  return true;
}

TypePtr Analyser::IsCompatible(ExpressionNodePtr const &node, ExpressionNodePtr const &lhs,
                               ExpressionNodePtr const &rhs)
{
  Node::Kind const op = node->kind;
  if ((lhs->type->id == TypeIds::Void) || (lhs->type->id == TypeIds::Null))
  {
    AddError(node->token, "incompatible types");
    return nullptr;
  }
  if ((rhs->type->id == TypeIds::Void) || (rhs->type->id == TypeIds::Null))
  {
    AddError(node->token, "incompatible types");
    return nullptr;
  }
  bool const lhs_is_primitive     = lhs->type->category == TypeCategory::Primitive;
  bool const rhs_is_primitive     = rhs->type->category == TypeCategory::Primitive;
  bool const lhs_is_instantiation = lhs->type->category == TypeCategory::TemplateInstantiation;
  bool const rhs_is_instantiation = rhs->type->category == TypeCategory::TemplateInstantiation;
  if (lhs_is_primitive)
  {
    if (rhs_is_primitive)
    {
      // primitive op primitive
      if (IsNumberType(lhs->type) && (lhs->type == rhs->type))
      {
        return lhs->type;
      }
    }
    else
    {
      // primitive op object
      if (rhs_is_instantiation && IsLeftOpEnabled(rhs->type, op))
      {
        TypePtr const &rhs_type = rhs->type->types[0];
        if (lhs->type == rhs_type)
        {
          return rhs->type;
        }
      }
    }
  }
  else
  {
    if (rhs_is_primitive)
    {
      // object op primitive
      if (lhs_is_instantiation && IsRightOpEnabled(lhs->type, op))
      {
        TypePtr const &lhs_type = lhs->type->types[0];
        if (lhs_type == rhs->type)
        {
          return lhs->type;
        }
      }
    }
    else
    {
      // object op object
      if ((lhs->type == rhs->type) && IsOpEnabled(lhs->type, op))
      {
        return lhs->type;
      }
    }
  }
  AddError(node->token, "incompatible types");
  return nullptr;
}

TypePtr Analyser::ConvertType(TypePtr const &type, TypePtr const &instantiated_template_type)
{
  TypePtr converted_type;
  if (type->category == TypeCategory::Template)
  {
    converted_type = instantiated_template_type;
  }
  else if (type->id == TypeIds::TemplateParameter1)
  {
    converted_type = instantiated_template_type->types[0];
  }
  else if (type->id == TypeIds::TemplateParameter2)
  {
    converted_type = instantiated_template_type->types[1];
  }
  else
  {
    converted_type = type;
  }
  return converted_type;
}

bool Analyser::MatchType(TypePtr const &supplied_type, TypePtr const &expected_type) const
{
  if (expected_type->category == TypeCategory::Variant)
  {
    for (auto const &possible_type : expected_type->types)
    {
      if (supplied_type == possible_type)
      {
        return true;
      }
    }
  }
  else if (supplied_type == expected_type)
  {
    return true;
  }
  return false;
}

bool Analyser::MatchTypes(TypePtr const &type, TypePtrArray const &supplied_types,
                          TypePtrArray const &expected_types, TypePtrArray &actual_types)
{
  actual_types.clear();
  size_t const num_types = expected_types.size();
  if (supplied_types.size() != num_types)
  {
    // Not a match
    return false;
  }
  for (size_t i = 0; i < num_types; ++i)
  {
    TypePtr const &supplied_type = supplied_types[i];
    if (supplied_type->id == TypeIds::Void)
    {
      // Not a match
      return false;
    }
    TypePtr expected_type = ConvertType(expected_types[i], type);
    if (supplied_type->id == TypeIds::Null)
    {
      if (expected_type->category == TypeCategory::Primitive)
      {
        // Not a match
        return false;
      }
      actual_types.push_back(expected_type);
    }
    else
    {
      if (!MatchType(supplied_type, expected_type))
      {
        // Not a match
        return false;
      }
      actual_types.push_back(supplied_type);
    }
  }
  // Got a match
  return true;
}

FunctionPtr Analyser::FindFunction(TypePtr const &type, FunctionGroupPtr const &fg,
                                   TypePtrArray const &supplied_types, TypePtrArray &actual_types)
{
  // type is nullptr if fg is a user function or is an opcode-invoked free function
  FunctionPtrArray          functions;
  std::vector<TypePtrArray> array;
  for (FunctionPtr const &function : fg->functions)
  {
    TypePtrArray temp_actual_types;
    if (MatchTypes(type, supplied_types, function->parameter_types, temp_actual_types))
    {
      array.push_back(temp_actual_types);
      functions.push_back(function);
    }
  }
  if (functions.size() == 1)
  {
    actual_types = array[0];
    return functions[0];
  }
  // Matching function not found, or ambiguous
  return nullptr;
}

TypePtr Analyser::FindType(ExpressionNodePtr const &node)
{
  SymbolPtr symbol = FindSymbol(node);
  if (symbol == nullptr)
  {
    return nullptr;
  }
  if (!symbol->IsType())
  {
    return nullptr;
  }
  return ConvertToTypePtr(symbol);
}

SymbolPtr Analyser::FindSymbol(ExpressionNodePtr const &node)
{
  SymbolPtr symbol;
  if (node->kind == Node::Kind::Template)
  {
    symbol = SearchSymbolTables(node->token.text);
    if (symbol)
    {
      // Template instantiation already exists
      return ConvertToTypePtr(symbol);
    }
    ExpressionNodePtr  identifier_node = ConvertToExpressionNodePtr(node->children[0]);
    std::string const &name            = identifier_node->token.text;
    symbol                             = SearchSymbolTables(name);
    if (symbol == nullptr)
    {
      // Template type doesn't exist
      return nullptr;
    }
    TypePtr      template_type                = ConvertToTypePtr(symbol);
    size_t const num_supplied_parameter_types = node->children.size() - 1;
    size_t const num_expected_parameter_types = template_type->types.size();
    TypePtrArray parameter_types;
    if (num_supplied_parameter_types != num_expected_parameter_types)
    {
      return nullptr;
    }
    for (size_t i = 1; i <= num_expected_parameter_types; ++i)
    {
      ExpressionNodePtr parameter_type_node = ConvertToExpressionNodePtr(node->children[i]);
      TypePtr           parameter_type      = FindType(parameter_type_node);
      if (parameter_type == nullptr)
      {
        return nullptr;
      }
      TypePtr const &expected_parameter_type = template_type->types[i - 1];
      if (expected_parameter_type->id != TypeIds::Any)
      {
        if (!MatchType(parameter_type, expected_parameter_type))
        {
          return nullptr;
        }
      }
      // Need to check here that parameter_type does in fact support any operator(s)
      // required by the template_type's i'th type parameter...
      parameter_types.push_back(parameter_type);
    }
    TypePtr type;
    CreateTemplateInstantiationType(next_instantiation_type_id_++, template_type, parameter_types,
                                    type);
    return type;
  }
  else  // (node->kind == Node::Kind::Identifier)
  {
    std::string const &name = node->token.text;
    symbol                  = SearchSymbolTables(name);
    if (symbol)
    {
      return symbol;
    }
    return nullptr;
  }
}

SymbolPtr Analyser::SearchSymbolTables(std::string const &name)
{
  SymbolPtr symbol = global_symbol_table_->Find(name);
  if (symbol)
  {
    return symbol;
  }
  auto it  = blocks_.rbegin();
  auto end = blocks_.rend();
  while (it != end)
  {
    BlockNodePtr const &block_node = *it;
    symbol                         = block_node->symbol_table->Find(name);
    if (symbol)
    {
      return symbol;
    }
    ++it;
  }
  // Name not found in any symbol table
  return nullptr;
}

void Analyser::SetVariable(ExpressionNodePtr const &node, VariablePtr const &variable)
{
  node->category = ExpressionNode::Category::Variable;
  node->variable = variable;
  node->type     = variable->type;
  node->fg       = nullptr;
  node->function = nullptr;
}

void Analyser::SetLV(ExpressionNodePtr const &node, TypePtr const &type)
{
  node->category = ExpressionNode::Category::LV;
  node->variable = nullptr;
  node->type     = type;
  node->fg       = nullptr;
  node->function = nullptr;
}

void Analyser::SetRV(ExpressionNodePtr const &node, TypePtr const &type)
{
  node->category = ExpressionNode::Category::RV;
  node->variable = nullptr;
  node->type     = type;
  node->fg       = nullptr;
  node->function = nullptr;
}

void Analyser::SetType(ExpressionNodePtr const &node, TypePtr const &type)
{
  node->category = ExpressionNode::Category::Type;
  node->variable = nullptr;
  node->type     = type;
  node->fg       = nullptr;
  node->function = nullptr;
}

void Analyser::SetFunction(ExpressionNodePtr const &node, FunctionGroupPtr const &fg,
                           TypePtr const &fg_owner, bool function_invoked_on_instance)
{
  node->category                     = ExpressionNode::Category::Function;
  node->variable                     = nullptr;
  node->type                         = fg_owner;
  node->fg                           = fg;
  node->function_invoked_on_instance = function_invoked_on_instance;
  node->function                     = nullptr;
}

void Analyser::CreateMetaType(std::string const &name, TypeId id, TypePtr &type)
{
  type = CreateType(name, id, TypeCategory::Meta);
  AddType(id, type, TypeInfo(name, id, TypeCategory::Meta, {}));
}

void Analyser::CreatePrimitiveType(std::string const &name, TypeId id,
                                   bool add_to_global_symbol_table, TypePtr &type)
{
  type = CreateType(name, id, TypeCategory::Primitive);
  AddType(id, type, TypeInfo(name, id, TypeCategory::Primitive, {}));
  if (add_to_global_symbol_table)
  {
    global_symbol_table_->Add(name, type);
  }
}

void Analyser::CreateClassType(std::string const &name, TypeId id, TypePtr &type)
{
  type = CreateType(name, id, TypeCategory::Class);
  AddType(id, type, TypeInfo(name, id, TypeCategory::Class, {}));
  type->symbol_table = CreateSymbolTable();
  global_symbol_table_->Add(name, type);
}

void Analyser::CreateTemplateType(std::string const &name, TypeId id,
                                  TypePtrArray const &allowed_types, TypePtr &type)
{
  type = CreateType(name, id, TypeCategory::Template);
  AddType(id, type, TypeInfo(name, id, TypeCategory::Template, {}));
  type->symbol_table = CreateSymbolTable();
  type->types        = allowed_types;
  global_symbol_table_->Add(name, type);
}

void Analyser::CreateTemplateInstantiationType(TypeId id, TypePtr const &template_type,
                                               TypePtrArray const &parameter_types, TypePtr &type)
{
  TypeIdArray parameter_type_ids;
  for (TypePtr const &parameter_type : parameter_types)
  {
    parameter_type_ids.push_back(parameter_type->id);
  }
  InternalCreateTemplateInstantiationType(id, template_type, parameter_types, parameter_type_ids,
                                          type);
}

void Analyser::CreateVariantType(std::string const &name, TypeId id,
                                 TypePtrArray const &allowed_types, TypePtr &type)
{
  type = CreateType(name, id, TypeCategory::Variant);
  AddType(id, type, TypeInfo(name, id, TypeCategory::Variant, {}));
  type->types = allowed_types;
}

void Analyser::InternalCreateTemplateInstantiationType(TypeId id, TypePtr const &template_type,
                                                       TypePtrArray const &parameter_types,
                                                       TypeIdArray const & parameter_type_ids,
                                                       TypePtr &           type)
{
  std::stringstream stream;
  stream << template_type->name + "<";
  size_t const last = parameter_types.size() - 1;
  for (size_t i = 0; i <= last; ++i)
  {
    stream << parameter_types[i]->name;
    if (i < last)
    {
      stream << ", ";
    }
  }
  stream << ">";
  std::string name = stream.str();
  type             = CreateType(name, id, TypeCategory::TemplateInstantiation);
  AddType(id, type, TypeInfo(name, id, TypeCategory::TemplateInstantiation, parameter_type_ids));
  type->template_type = template_type;
  type->types         = parameter_types;
  global_symbol_table_->Add(name, type);
}

void Analyser::CreateOpcodeFreeFunction(std::string const &name, Opcode opcode,
                                        TypePtrArray const &parameter_types,
                                        TypePtr const &     return_type)
{
  FunctionPtr f = CreateOpcodeFunction(name, Function::Kind::OpcodeFreeFunction, opcode,
                                       parameter_types, return_type);
  AddFunction(global_symbol_table_, f);
}

void Analyser::CreateOpcodeTypeConstructor(TypePtr const &type, Opcode opcode,
                                           TypePtrArray const &parameter_types)
{
  FunctionPtr f = CreateOpcodeFunction(CONSTRUCTOR, Function::Kind::OpcodeTypeFunction, opcode,
                                       parameter_types, type);
  AddFunction(type->symbol_table, f);
}

void Analyser::CreateOpcodeTypeFunction(TypePtr const &type, std::string const &name, Opcode opcode,
                                        TypePtrArray const &parameter_types,
                                        TypePtr const &     return_type)
{
  FunctionPtr f = CreateOpcodeFunction(name, Function::Kind::OpcodeTypeFunction, opcode,
                                       parameter_types, return_type);
  AddFunction(type->symbol_table, f);
}

void Analyser::CreateOpcodeInstanceFunction(TypePtr const &type, std::string const &name,
                                            Opcode opcode, TypePtrArray const &parameter_types,
                                            TypePtr const &return_type)
{
  FunctionPtr f = CreateOpcodeFunction(name, Function::Kind::OpcodeInstanceFunction, opcode,
                                       parameter_types, return_type);
  AddFunction(type->symbol_table, f);
}

FunctionPtr Analyser::CreateUserFunction(std::string const &     name,
                                         TypePtrArray const &    parameter_types,
                                         VariablePtrArray const &parameter_variables,
                                         TypePtr const &         return_type)
{
  FunctionPtr function          = CreateFunction(name, Function::Kind::UserFreeFunction);
  function->opcode              = Opcodes::Unknown;
  function->index               = 0;
  function->parameter_types     = parameter_types;
  function->parameter_variables = parameter_variables;
  function->return_type         = return_type;
  return function;
}

FunctionPtr Analyser::CreateOpcodeFunction(std::string const &name, Function::Kind kind,
                                           Opcode opcode, TypePtrArray const &parameter_types,
                                           TypePtr const &return_type)
{
  FunctionPtr function      = CreateFunction(name, kind);
  function->opcode          = opcode;
  function->index           = 0;
  function->parameter_types = parameter_types;
  function->return_type     = return_type;
  return function;
}

void Analyser::AddFunction(SymbolTablePtr const &symbol_table, FunctionPtr const &function)
{
  FunctionGroupPtr fg;
  SymbolPtr        symbol = symbol_table->Find(function->name);
  if (symbol)
  {
    // Function group already exists
    fg = ConvertToFunctionGroupPtr(symbol);
  }
  else
  {
    // Create new function group
    fg = CreateFunctionGroup(function->name);
    symbol_table->Add(function->name, fg);
  }
  // Add the function to the function group
  fg->functions.push_back(function);
}

}  // namespace vm
}  // namespace fetch
