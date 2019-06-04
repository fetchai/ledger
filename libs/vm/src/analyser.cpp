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
#include "vm/array.hpp"
#include "vm/map.hpp"
#include "vm/matrix.hpp"
#include "vm/persistent_map.hpp"
#include "vm/state.hpp"
#include "vm/string.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>

namespace fetch {
namespace vm {

std::string Analyser::CONSTRUCTOR       = "[Constructor]";
std::string Analyser::GET_INDEXED_VALUE = "[GetIndexedValue]";
std::string Analyser::SET_INDEXED_VALUE = "[SetIndexedValue]";

void Analyser::Initialise()
{
  operator_map_        = {{NodeKind::Equal, Operator::Equal},
                   {NodeKind::NotEqual, Operator::NotEqual},
                   {NodeKind::LessThan, Operator::LessThan},
                   {NodeKind::LessThanOrEqual, Operator::LessThanOrEqual},
                   {NodeKind::GreaterThan, Operator::GreaterThan},
                   {NodeKind::GreaterThanOrEqual, Operator::GreaterThanOrEqual},
                   {NodeKind::Negate, Operator::Negate},
                   {NodeKind::Add, Operator::Add},
                   {NodeKind::Subtract, Operator::Subtract},
                   {NodeKind::Multiply, Operator::Multiply},
                   {NodeKind::Divide, Operator::Divide},
                   {NodeKind::InplaceAdd, Operator::InplaceAdd},
                   {NodeKind::InplaceSubtract, Operator::InplaceSubtract},
                   {NodeKind::InplaceMultiply, Operator::InplaceMultiply},
                   {NodeKind::InplaceDivide, Operator::InplaceDivide}};
  type_map_            = TypeMap();
  type_info_array_     = TypeInfoArray(TypeIds::NumReserved);
  type_info_map_       = TypeInfoMap();
  registered_types_    = RegisteredTypes();
  function_info_array_ = FunctionInfoArray();
  symbols_             = CreateSymbolTable();
  CreatePrimitiveType("Null", TypeIndex(typeid(std::nullptr_t)), false, TypeIds::Null, null_type_);
  CreatePrimitiveType("Void", TypeIndex(typeid(void)), false, TypeIds::Void, void_type_);
  CreatePrimitiveType("Bool", TypeIndex(typeid(bool)), true, TypeIds::Bool, bool_type_);
  CreatePrimitiveType("Int8", TypeIndex(typeid(int8_t)), true, TypeIds::Int8, int8_type_);
  CreatePrimitiveType("Byte", TypeIndex(typeid(uint8_t)), true, TypeIds::Byte, byte_type_);
  CreatePrimitiveType("Int16", TypeIndex(typeid(int16_t)), true, TypeIds::Int16, int16_type_);
  CreatePrimitiveType("UInt16", TypeIndex(typeid(uint16_t)), true, TypeIds::UInt16, uint16_type_);
  CreatePrimitiveType("Int32", TypeIndex(typeid(int32_t)), true, TypeIds::Int32, int32_type_);
  CreatePrimitiveType("UInt32", TypeIndex(typeid(uint32_t)), true, TypeIds::UInt32, uint32_type_);
  CreatePrimitiveType("Int64", TypeIndex(typeid(int64_t)), true, TypeIds::Int64, int64_type_);
  CreatePrimitiveType("UInt64", TypeIndex(typeid(uint64_t)), true, TypeIds::UInt64, uint64_type_);
  CreatePrimitiveType("Float32", TypeIndex(typeid(float)), true, TypeIds::Float32, float32_type_);
  CreatePrimitiveType("Float64", TypeIndex(typeid(double)), true, TypeIds::Float64, float64_type_);
  CreateClassType("String", TypeIndex(typeid(String)), TypeIds::String, string_type_);
  EnableOperator(string_type_, Operator::Equal);
  EnableOperator(string_type_, Operator::NotEqual);
  EnableOperator(string_type_, Operator::LessThan);
  EnableOperator(string_type_, Operator::LessThanOrEqual);
  EnableOperator(string_type_, Operator::GreaterThan);
  EnableOperator(string_type_, Operator::GreaterThanOrEqual);
  EnableOperator(string_type_, Operator::Add);

  CreateClassType("Address", TypeIndex(typeid(Address)), TypeIds::Address, address_type_);
  EnableOperator(address_type_, Operator::Equal);
  EnableOperator(address_type_, Operator::NotEqual);

  CreateMetaType("[TemplateParameter1]", TypeIndex(typeid(TemplateParameter1)), TypeIds::Unknown,
                 template_parameter1_type_);
  CreateMetaType("[TemplateParameter2]", TypeIndex(typeid(TemplateParameter2)), TypeIds::Unknown,
                 template_parameter2_type_);
  CreateMetaType("[Any]", TypeIndex(typeid(Any)), TypeIds::Unknown, any_type_);
  CreateMetaType("[AnyPrimitive]", TypeIndex(typeid(AnyPrimitive)), TypeIds::Unknown,
                 any_primitive_type_);

  EnableOperator(bool_type_, Operator::Equal);
  EnableOperator(bool_type_, Operator::NotEqual);

  TypePtrArray const integer_types = {int8_type_,  byte_type_,   int16_type_, uint16_type_,
                                      int32_type_, uint32_type_, int64_type_, uint64_type_};
  TypePtrArray const number_types  = {int8_type_,    byte_type_,   int16_type_, uint16_type_,
                                     int32_type_,   uint32_type_, int64_type_, uint64_type_,
                                     float32_type_, float64_type_};
  for (auto const &type : number_types)
  {
    EnableOperator(type, Operator::Equal);
    EnableOperator(type, Operator::NotEqual);
    EnableOperator(type, Operator::LessThan);
    EnableOperator(type, Operator::LessThanOrEqual);
    EnableOperator(type, Operator::GreaterThan);
    EnableOperator(type, Operator::GreaterThanOrEqual);
    EnableOperator(type, Operator::Negate);
    EnableOperator(type, Operator::Add);
    EnableOperator(type, Operator::Subtract);
    EnableOperator(type, Operator::Multiply);
    EnableOperator(type, Operator::Divide);
    EnableOperator(type, Operator::InplaceAdd);
    EnableOperator(type, Operator::InplaceSubtract);
    EnableOperator(type, Operator::InplaceMultiply);
    EnableOperator(type, Operator::InplaceDivide);
  }
  CreateGroupType("[AnyInteger]", TypeIndex(typeid(AnyInteger)), integer_types, TypeIds::Unknown,
                  any_integer_type_);
  CreateGroupType("[AnyFloatingPoint]", TypeIndex(typeid(AnyFloatingPoint)),
                  {float32_type_, float64_type_}, TypeIds::Unknown, any_floating_point_type_);
  CreateTemplateType("Matrix", TypeIndex(typeid(IMatrix)), {any_floating_point_type_},
                     TypeIds::Unknown, matrix_type_);
  EnableOperator(matrix_type_, Operator::Negate);
  EnableOperator(matrix_type_, Operator::Add);
  EnableOperator(matrix_type_, Operator::Subtract);
  EnableOperator(matrix_type_, Operator::Multiply);
  EnableOperator(matrix_type_, Operator::InplaceAdd);
  EnableOperator(matrix_type_, Operator::InplaceSubtract);
  EnableLeftOperator(matrix_type_, Operator::Multiply);
  EnableRightOperator(matrix_type_, Operator::Add);
  EnableRightOperator(matrix_type_, Operator::Subtract);
  EnableRightOperator(matrix_type_, Operator::Multiply);
  EnableRightOperator(matrix_type_, Operator::Divide);
  EnableRightOperator(matrix_type_, Operator::InplaceAdd);
  EnableRightOperator(matrix_type_, Operator::InplaceSubtract);
  EnableRightOperator(matrix_type_, Operator::InplaceMultiply);
  EnableRightOperator(matrix_type_, Operator::InplaceDivide);
  CreateTemplateType("Array", TypeIndex(typeid(IArray)), {any_type_}, TypeIds::Unknown,
                     array_type_);
  CreateTemplateType("Map", TypeIndex(typeid(IMap)), {any_type_, any_type_}, TypeIds::Unknown,
                     map_type_);
  CreateTemplateType("State", TypeIndex(typeid(IState)), {any_type_}, TypeIds::Unknown,
                     state_type_);

  CreateTemplateType("ShardedState", TypeIndex(typeid(IShardedState)), {any_type_},
                     TypeIds::Unknown, persistent_map_type_);
}

void Analyser::UnInitialise()
{
  operator_map_ = OperatorMap();
  type_map_.Reset();
  type_map_            = TypeMap();
  type_info_array_     = TypeInfoArray();
  type_info_map_       = TypeInfoMap();
  registered_types_    = RegisteredTypes();
  function_info_array_ = FunctionInfoArray();
  if (symbols_)
  {
    symbols_->Reset();
    symbols_ = nullptr;
  }
  null_type_                = nullptr;
  void_type_                = nullptr;
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
  string_type_              = nullptr;
  address_type_             = nullptr;
  template_parameter1_type_ = nullptr;
  template_parameter2_type_ = nullptr;
  any_type_                 = nullptr;
  any_primitive_type_       = nullptr;
  any_integer_type_         = nullptr;
  any_floating_point_type_  = nullptr;
  matrix_type_              = nullptr;
  array_type_               = nullptr;
  map_type_                 = nullptr;
  state_type_               = nullptr;
  address_type_             = nullptr;
  persistent_map_type_      = nullptr;
}

void Analyser::CreateClassType(std::string const &name, TypeIndex type_index)
{
  TypePtr type;
  CreateClassType(name, type_index, TypeIds::Unknown, type);
}

void Analyser::CreateInstantiationType(TypeIndex type_index, TypeIndex template_type_index,
                                       TypeIndexArray const &parameter_type_index_array)
{
  TypePtr type;
  CreateInstantiationType(type_index, GetType(template_type_index),
                          GetTypes(parameter_type_index_array), TypeIds::Unknown, type);
}

void Analyser::CreateFreeFunction(std::string const &   name,
                                  TypeIndexArray const &parameter_type_index_array,
                                  TypeIndex return_type_index, Handler const &handler)
{
  CreateFreeFunction(name, GetTypes(parameter_type_index_array), GetType(return_type_index),
                     handler);
}

void Analyser::CreateConstructor(TypeIndex             type_index,
                                 TypeIndexArray const &parameter_type_index_array,
                                 Handler const &       handler)
{
  CreateConstructor(GetType(type_index), GetTypes(parameter_type_index_array), handler);
}

void Analyser::CreateStaticMemberFunction(TypeIndex type_index, std::string const &function_name,
                                          TypeIndexArray const &parameter_type_index_array,
                                          TypeIndex return_type_index, Handler const &handler)
{
  CreateStaticMemberFunction(GetType(type_index), function_name,
                             GetTypes(parameter_type_index_array), GetType(return_type_index),
                             handler);
}

void Analyser::CreateMemberFunction(TypeIndex type_index, std::string const &function_name,
                                    TypeIndexArray const &parameter_type_index_array,
                                    TypeIndex return_type_index, Handler const &handler)
{
  CreateMemberFunction(GetType(type_index), function_name, GetTypes(parameter_type_index_array),
                       GetType(return_type_index), handler);
}

void Analyser::EnableOperator(TypeIndex type_index, Operator op)
{
  EnableOperator(GetType(type_index), op);
}

void Analyser::EnableIndexOperator(TypeIndex             type_index,
                                   TypeIndexArray const &input_type_index_array,
                                   TypeIndex output_type_index, Handler const &get_handler,
                                   Handler const &set_handler)
{
  EnableIndexOperator(GetType(type_index), GetTypes(input_type_index_array),
                      GetType(output_type_index), get_handler, set_handler);
}

bool Analyser::Analyse(BlockNodePtr const &root, std::vector<std::string> &errors)
{
  root_ = root;
  blocks_.clear();
  loops_.clear();
  function_ = nullptr;
  errors_.clear();

  root_->symbols = CreateSymbolTable();

  // Create symbol tables for all blocks
  // Check function prototypes
  BuildBlock(root_);

  if (!errors_.empty())
  {
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

  if (!errors_.empty())
  {
    errors = std::move(errors_);
    errors_.clear();
    return false;
  }
  errors.clear();
  return true;
}

void Analyser::AddError(uint16_t line, std::string const &message)
{
  std::stringstream stream;
  stream << "line " << line << ": "
         << "error: " << message;
  errors_.push_back(stream.str());
}

void Analyser::BuildBlock(BlockNodePtr const &block_node)
{
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      BuildFile(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::FunctionDefinitionStatement:
    {
      BuildFunctionDefinition(block_node, ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::WhileStatement:
    {
      BuildWhileStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::ForStatement:
    {
      BuildForStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::IfStatement:
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

void Analyser::BuildFile(BlockNodePtr const &file_node)
{
  file_node->symbols = CreateSymbolTable();
  BuildBlock(file_node);
}

void Analyser::BuildFunctionDefinition(BlockNodePtr const &parent_block_node,
                                       BlockNodePtr const &function_definition_node)
{
  function_definition_node->symbols = CreateSymbolTable();
  ExpressionNodePtr identifier_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  std::string const &name  = identifier_node->text;
  int const          count = static_cast<int>(function_definition_node->children.size());
  VariablePtrArray   parameter_variables;
  TypePtrArray       parameter_types;
  int const          num_parameters = int((count - 3) / 2);
  int                problems       = 0;
  for (int i = 0; i < num_parameters; ++i)
  {
    ExpressionNodePtr parameter_node =
        ConvertToExpressionNodePtr(function_definition_node->children[std::size_t(2 + i * 2)]);
    std::string const &parameter_name = parameter_node->text;
    SymbolPtr          symbol         = function_definition_node->symbols->Find(parameter_name);
    if (symbol)
    {
      std::stringstream stream;
      AddError(parameter_node->line, "parameter name '" + parameter_name + "' is already defined");
      ++problems;
      continue;
    }
    ExpressionNodePtr parameter_type_node =
        ConvertToExpressionNodePtr(function_definition_node->children[std::size_t(3 + i * 2)]);
    TypePtr parameter_type = FindType(parameter_type_node);
    if (parameter_type == nullptr)
    {
      AddError(parameter_type_node->line, "unknown type '" + parameter_type_node->text + "'");
      ++problems;
      continue;
    }
    VariablePtr parameter_variable = CreateVariable(VariableKind::Parameter, parameter_name);
    parameter_variable->type       = parameter_type;
    function_definition_node->symbols->Add(parameter_variable);
    parameter_node->variable = parameter_variable;
    parameter_node->type     = parameter_variable->type;
    parameter_variables.push_back(parameter_variable);
    parameter_types.push_back(parameter_type);
  }
  TypePtr           return_type;
  ExpressionNodePtr return_type_node =
      ConvertToExpressionNodePtr(function_definition_node->children[std::size_t(count - 1)]);
  if (return_type_node)
  {
    return_type = FindType(return_type_node);
    if (return_type == nullptr)
    {
      AddError(return_type_node->line, "unknown type '" + return_type_node->text + "'");
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
  SymbolPtr        symbol = parent_block_node->symbols->Find(name);
  if (symbol)
  {
    fg = ConvertToFunctionGroupPtr(symbol);
    TypePtrArray dummy;
    if (FindFunction(nullptr, fg, parameter_types, dummy))
    {
      AddError(function_definition_node->line,
               "function '" + name + "' is already defined with the same parameter types");
      return;
    }
  }
  else
  {
    fg = CreateFunctionGroup(name);
    parent_block_node->symbols->Add(fg);
  }
  FunctionPtr function =
      CreateUserDefinedFreeFunction(name, parameter_types, parameter_variables, return_type);
  fg->functions.push_back(function);
  identifier_node->function = function;
  BuildBlock(function_definition_node);
}

void Analyser::BuildWhileStatement(BlockNodePtr const &while_statement_node)
{
  while_statement_node->symbols = CreateSymbolTable();
  BuildBlock(while_statement_node);
}

void Analyser::BuildForStatement(BlockNodePtr const &for_statement_node)
{
  for_statement_node->symbols = CreateSymbolTable();
  BuildBlock(for_statement_node);
}

void Analyser::BuildIfStatement(NodePtr const &if_statement_node)
{
  for (NodePtr const &child : if_statement_node->children)
  {
    BlockNodePtr block_node = ConvertToBlockNodePtr(child);
    block_node->symbols     = CreateSymbolTable();
    BuildBlock(block_node);
  }
}

void Analyser::AnnotateBlock(BlockNodePtr const &block_node)
{
  blocks_.push_back(block_node);
  bool const is_loop = ((block_node->node_kind == NodeKind::WhileStatement) ||
                        (block_node->node_kind == NodeKind::ForStatement));
  if (is_loop)
  {
    loops_.push_back(block_node);
  }
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      AnnotateFile(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::FunctionDefinitionStatement:
    {
      AnnotateFunctionDefinitionStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::WhileStatement:
    {
      AnnotateWhileStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::ForStatement:
    {
      AnnotateForStatement(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::IfStatement:
    {
      AnnotateIfStatement(child);
      break;
    }
    case NodeKind::VarDeclarationStatement:
    case NodeKind::VarDeclarationTypedAssignmentStatement:
    case NodeKind::VarDeclarationTypelessAssignmentStatement:
    {
      AnnotateVarStatement(block_node, child);
      break;
    }
    case NodeKind::ReturnStatement:
    {
      AnnotateReturnStatement(child);
      break;
    }
    case NodeKind::BreakStatement:
    {
      if (!loops_.empty())
      {
        AddError(child->line, "break statement is not inside a while or for loop");
      }
      break;
    }
    case NodeKind::ContinueStatement:
    {
      if (!loops_.empty())
      {
        AddError(child->line, "continue statement is not inside a while or for loop");
      }
      break;
    }
    case NodeKind::Assign:
    {
      AnnotateAssignOp(ConvertToExpressionNodePtr(child));
      break;
    }
    case NodeKind::InplaceAdd:
    case NodeKind::InplaceSubtract:
    case NodeKind::InplaceMultiply:
    case NodeKind::InplaceDivide:
    {
      AnnotateInplaceArithmeticOp(ConvertToExpressionNodePtr(child));
      break;
    }
    case NodeKind::InplaceModulo:
    {
      AnnotateInplaceModuloOp(ConvertToExpressionNodePtr(child));
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

void Analyser::AnnotateFile(BlockNodePtr const &file_node)
{
  AnnotateBlock(file_node);
}

void Analyser::AnnotateFunctionDefinitionStatement(BlockNodePtr const &function_definition_node)
{
  ExpressionNodePtr identifier_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  function_ = identifier_node->function;
  AnnotateBlock(function_definition_node);
  if (errors_.size() == 0)
  {
    if (function_->return_type->IsVoid() == false)
    {
      if (TestBlock(function_definition_node))
      {
        AddError(identifier_node->line,
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
  std::string const &name            = identifier_node->text;
  // Note: variable is created with no type
  VariablePtr variable = CreateVariable(VariableKind::For, name);
  for_statement_node->symbols->Add(variable);
  identifier_node->variable            = variable;
  std::size_t const              count = for_statement_node->children.size() - 1;
  std::vector<ExpressionNodePtr> nodes;
  int                            problems = 0;
  for (std::size_t i = 1; i <= count; ++i)
  {
    NodePtr const &   child      = for_statement_node->children[i];
    ExpressionNodePtr child_node = ConvertToExpressionNodePtr(child);
    if (AnnotateExpression(child_node) == false)
    {
      ++problems;
      continue;
    }
    if (MatchType(child_node->type, any_integer_type_) == false)
    {
      ++problems;
      AddError(child_node->line, "integral type expected");
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
        AddError(nodes[1]->line, "incompatible types");
        ++problems;
      }
    }
    else
    {
      AddError(nodes[0]->line, "incompatible types");
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
    if (child_block_node->node_kind != NodeKind::Else)
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
  std::string const &name            = identifier_node->text;
  SymbolPtr          symbol          = parent_block_node->symbols->Find(name);
  if (symbol)
  {
    AddError(identifier_node->line, "variable '" + name + "' is already defined");
    return;
  }
  // Note: variable is created with no type
  VariablePtr variable = CreateVariable(VariableKind::Local, name);
  parent_block_node->symbols->Add(variable);
  identifier_node->variable = variable;
  if (var_statement_node->node_kind == NodeKind::VarDeclarationStatement)
  {
    ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (AnnotateTypeExpression(type_node) == false)
    {
      return;
    }
    variable->type = type_node->type;
  }
  else if (var_statement_node->node_kind == NodeKind::VarDeclarationTypedAssignmentStatement)
  {
    ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (AnnotateTypeExpression(type_node) == false)
    {
      return;
    }
    ExpressionNodePtr expression_node = ConvertToExpressionNodePtr(var_statement_node->children[2]);
    if (AnnotateExpression(expression_node) == false)
    {
      return;
    }
    if (expression_node->type->IsNull() == false)
    {
      if (type_node->type != expression_node->type)
      {
        AddError(type_node->line, "incompatible types");
        return;
      }
    }
    else
    {
      if (type_node->type->IsPrimitive())
      {
        // Can't assign null to a primitive type
        AddError(type_node->line, "unable to assign null to primitive type");
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
    if (AnnotateExpression(expression_node) == false)
    {
      return;
    }
    if (expression_node->type->IsVoid() || expression_node->type->IsNull())
    {
      AddError(expression_node->line, "unable to infer type");
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
    if (AnnotateExpression(expression_node) == false)
    {
      return;
    }
    if (expression_node->type->IsNull() == false)
    {
      // note: function_->return_type can be Void
      if (expression_node->type != function_->return_type)
      {
        AddError(expression_node->line, "type does not match function return type");
        return;
      }
    }
    else
    {
      // note: function_->return_type can be Void
      if (function_->return_type->IsPrimitive())
      {
        AddError(expression_node->line, "unable to return null");
        return;
      }
      // Convert the null type to the known return type of the function
      expression_node->type = function_->return_type;
    }
  }
  else
  {
    if (function_->return_type->IsVoid() == false)
    {
      AddError(return_statement_node->line, "return does not supply a value");
      return;
    }
  }
}

void Analyser::AnnotateConditionalBlock(BlockNodePtr const &conditional_node)
{
  ExpressionNodePtr expression_node = ConvertToExpressionNodePtr(conditional_node->children[0]);
  if (AnnotateExpression(expression_node))
  {
    if (expression_node->type != bool_type_)
    {
      AddError(expression_node->line, "boolean type expected");
    }
  }
  AnnotateBlock(conditional_node);
}

bool Analyser::AnnotateTypeExpression(ExpressionNodePtr const &node)
{
  TypePtr type = FindType(node);
  if (type == nullptr)
  {
    AddError(node->line, "unknown type '" + node->text + "'");
    return false;
  }
  SetTypeExpression(node, type);
  return true;
}

bool Analyser::AnnotateAssignOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (AnnotateLHSExpression(node, lhs) == false)
  {
    return false;
  }
  if (AnnotateExpression(rhs) == false)
  {
    return false;
  }
  if (rhs->type->IsVoid())
  {
    // Can't assign from a function with no return value
    AddError(node->line, "incompatible types");
    return false;
  }
  if (rhs->type->IsNull() == false)
  {
    if (lhs->type != rhs->type)
    {
      AddError(node->line, "incompatible types");
      return false;
    }
  }
  else
  {
    if (lhs->type->IsPrimitive())
    {
      // Can't assign null to a primitive type
      AddError(node->line, "incompatible types");
      return false;
    }
    // Convert the null type to the correct type
    rhs->type = lhs->type;
  }
  SetRVExpression(node, lhs->type);
  return true;
}

bool Analyser::AnnotateInplaceArithmeticOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (AnnotateLHSExpression(node, lhs) == false)
  {
    return false;
  }
  if (AnnotateExpression(rhs) == false)
  {
    return false;
  }
  return AnnotateArithmetic(node, lhs, rhs);
}

bool Analyser::AnnotateInplaceModuloOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (AnnotateLHSExpression(node, lhs) == false)
  {
    return false;
  }
  if (AnnotateExpression(rhs) == false)
  {
    return false;
  }
  if ((lhs->type != rhs->type) || (MatchType(lhs->type, any_integer_type_) == false))
  {
    AddError(node->line, "integral operands expected");
    return false;
  }
  SetRVExpression(node, lhs->type);
  return true;
}

bool Analyser::AnnotateLHSExpression(ExpressionNodePtr const &parent, ExpressionNodePtr const &lhs)
{
  if (AnnotateExpression(lhs) == false)
  {
    return false;
  }
  if (IsWriteable(lhs) == false)
  {
    return false;
  }
  bool const assigning_to_variable = lhs->IsVariableExpression();
  if (assigning_to_variable)
  {
    return true;
  }

  // lhs->node_kind == NodeKind::Index
  // Assigning to an indexed value...

  ExpressionNodePtr container_node = ConvertToExpressionNodePtr(lhs->children[0]);
  TypePtr           type;
  if (container_node->type->IsInstantiation())
  {
    type = container_node->type->template_type;
  }
  else
  {
    type = container_node->type;
  }

  std::size_t const num_supplied_indexes = lhs->children.size() - 1;
  TypePtrArray      supplied_index_types;
  for (std::size_t i = 1; i <= num_supplied_indexes; ++i)
  {
    NodePtr const &   supplied_index      = lhs->children[i];
    ExpressionNodePtr supplied_index_node = ConvertToExpressionNodePtr(supplied_index);
    supplied_index_types.push_back(supplied_index_node->type);
  }

  SymbolPtr setter_symbol = type->symbols->Find(SET_INDEXED_VALUE);
  if (setter_symbol == nullptr)
  {
    AddError(container_node->line, "unable to find matching index operator for type '" +
                                       container_node->type->name + "'");
    return false;
  }

  FunctionGroupPtr setter_fg = ConvertToFunctionGroupPtr(setter_symbol);

  TypePtrArray dummy;
  TypePtrArray setter_supplied_types = supplied_index_types;
  setter_supplied_types.push_back(lhs->type);
  FunctionPtr setter_f =
      FindFunction(container_node->type, setter_fg, setter_supplied_types, dummy);
  if (setter_f == nullptr)
  {
    AddError(container_node->line, "unable to find matching index operator for type '" +
                                       container_node->type->name + "'");
    return false;
  }

  parent->function = setter_f;
  return true;
}

bool Analyser::AnnotateExpression(ExpressionNodePtr const &node)
{
  switch (node->node_kind)
  {
  case NodeKind::Identifier:
  case NodeKind::Template:
  {
    SymbolPtr symbol = FindSymbol(node);
    if (symbol == nullptr)
    {
      AddError(node->line, "unknown symbol '" + node->text + "'");
      return false;
    }
    if (symbol->IsVariable() == false)
    {
      // Type name or function name
      AddError(node->line, "symbol '" + node->text + "' is not a variable");
      return false;
    }
    VariablePtr variable = ConvertToVariablePtr(symbol);
    if (variable->type == nullptr)
    {
      AddError(node->line, "variable '" + node->text + "' has unresolved type");
      return false;
    }
    SetVariableExpression(node, variable);
    break;
  }
  case NodeKind::Integer8:
  {
    SetRVExpression(node, int8_type_);
    break;
  }
  case NodeKind::UnsignedInteger8:
  {
    SetRVExpression(node, byte_type_);
    break;
  }
  case NodeKind::Integer16:
  {
    SetRVExpression(node, int16_type_);
    break;
  }
  case NodeKind::UnsignedInteger16:
  {
    SetRVExpression(node, uint16_type_);
    break;
  }
  case NodeKind::Integer32:
  {
    SetRVExpression(node, int32_type_);
    break;
  }
  case NodeKind::UnsignedInteger32:
  {
    SetRVExpression(node, uint32_type_);
    break;
  }
  case NodeKind::Integer64:
  {
    SetRVExpression(node, int64_type_);
    break;
  }
  case NodeKind::UnsignedInteger64:
  {
    SetRVExpression(node, uint64_type_);
    break;
  }
  case NodeKind::Float32:
  {
    SetRVExpression(node, float32_type_);
    break;
  }
  case NodeKind::Float64:
  {
    SetRVExpression(node, float64_type_);
    break;
  }
  case NodeKind::String:
  {
    SetRVExpression(node, string_type_);
    break;
  }
  case NodeKind::True:
  case NodeKind::False:
  {
    SetRVExpression(node, bool_type_);
    break;
  }
  case NodeKind::Null:
  {
    SetRVExpression(node, null_type_);
    break;
  }
  case NodeKind::Equal:
  case NodeKind::NotEqual:
  {
    if (AnnotateEqualityOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::LessThan:
  case NodeKind::LessThanOrEqual:
  case NodeKind::GreaterThan:
  case NodeKind::GreaterThanOrEqual:
  {
    if (AnnotateRelationalOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::And:
  case NodeKind::Or:
  {
    if (AnnotateBinaryLogicalOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::Not:
  {
    if (AnnotateUnaryLogicalOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::PrefixInc:
  case NodeKind::PrefixDec:
  case NodeKind::PostfixInc:
  case NodeKind::PostfixDec:
  {
    if (AnnotatePrefixPostfixOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::Negate:
  {
    if (AnnotateNegateOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::Add:
  case NodeKind::Subtract:
  case NodeKind::Multiply:
  case NodeKind::Divide:
  {
    if (AnnotateArithmeticOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::Modulo:
  {
    if (AnnotateModuloOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::Index:
  {
    if (AnnotateIndexOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::Dot:
  {
    if (AnnotateDotOp(node) == false)
    {
      return false;
    }
    break;
  }
  case NodeKind::Invoke:
  {
    if (AnnotateInvokeOp(node) == false)
    {
      return false;
    }
    break;
  }
  default:
  {
    AddError(node->line, "internal error at '" + node->text + "'");
    return false;
  }
  }  // switch
  return true;
}

bool Analyser::AnnotateEqualityOp(ExpressionNodePtr const &node)
{
  Operator const op = GetOperator(node->node_kind);
  for (NodePtr const &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false)
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (lhs->type->IsVoid() || rhs->type->IsVoid())
  {
    AddError(node->line, "unable to compare operand(s) of type Void");
    return false;
  }
  bool const lhs_is_concrete_type = !lhs->type->IsNull();
  bool const rhs_is_concrete_type = !rhs->type->IsNull();
  if (lhs_is_concrete_type)
  {
    bool const lhs_is_primitive = lhs->type->IsPrimitive();
    if (rhs_is_concrete_type)
    {
      if (lhs->type != rhs->type)
      {
        AddError(node->line, "incompatible types");
        return false;
      }
      bool const enabled = IsOperatorEnabled(lhs->type, op);
      if (enabled == false)
      {
        AddError(node->line, "operator not supported");
        return false;
      }
    }
    else
    {
      if (lhs_is_primitive)
      {
        // unable to compare LHS primitive type to RHS null
        AddError(node->line, "incompatible types");
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
      bool const rhs_is_primitive = rhs->type->IsPrimitive();
      if (rhs_is_primitive)
      {
        // unable to compare LHS null to RHS primitive type
        AddError(node->line, "incompatible types");
        return false;
      }
      // Convert the LHS null type to the correct type
      lhs->type = rhs->type;
    }
    else
    {
      // Comparing two nulls...
      // Type-uninferable nulls will be transformed to boolean true
      lhs->type = bool_type_;
      rhs->type = bool_type_;
    }
  }
  SetRVExpression(node, bool_type_);
  return true;
}

bool Analyser::AnnotateRelationalOp(ExpressionNodePtr const &node)
{
  Operator const op = GetOperator(node->node_kind);
  for (NodePtr const &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false)
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (lhs->type != rhs->type)
  {
    AddError(node->line, "incompatible types");
    return false;
  }
  bool const enabled = IsOperatorEnabled(lhs->type, op);
  if (enabled == false)
  {
    AddError(node->line, "operator not supported");
    return false;
  }
  SetRVExpression(node, bool_type_);
  return true;
}

bool Analyser::AnnotateBinaryLogicalOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false)
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if ((lhs->type != bool_type_) || (rhs->type != bool_type_))
  {
    AddError(node->line, "boolean operands expected");
    return false;
  }
  SetRVExpression(node, bool_type_);
  return true;
}

bool Analyser::AnnotateUnaryLogicalOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (AnnotateExpression(operand) == false)
  {
    return false;
  }
  if (operand->type != bool_type_)
  {
    AddError(node->line, "boolean operand expected");
    return false;
  }
  SetRVExpression(node, bool_type_);
  return true;
}

bool Analyser::AnnotatePrefixPostfixOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (AnnotateLHSExpression(node, operand) == false)
  {
    return false;
  }
  if (MatchType(operand->type, any_integer_type_) == false)
  {
    AddError(node->line, "integral type expected");
    return false;
  }
  SetRVExpression(node, operand->type);
  return true;
}

bool Analyser::AnnotateNegateOp(ExpressionNodePtr const &node)
{
  Operator const    op      = GetOperator(node->node_kind);
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (AnnotateExpression(operand) == false)
  {
    return false;
  }
  if (IsOperatorEnabled(operand->type, op) == false)
  {
    AddError(node->line, "operator not supported");
    return false;
  }
  SetRVExpression(node, operand->type);
  return true;
}

bool Analyser::AnnotateArithmeticOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false)
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  return AnnotateArithmetic(node, lhs, rhs);
}

bool Analyser::AnnotateModuloOp(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false)
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if ((lhs->type != rhs->type) || (MatchType(lhs->type, any_integer_type_) == false))
  {
    AddError(node->line, "integral operands expected");
    return false;
  }
  SetRVExpression(node, lhs->type);
  return true;
}

bool Analyser::AnnotateIndexOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  if ((lhs->node_kind == NodeKind::Identifier) || (lhs->node_kind == NodeKind::Template))
  {
    SymbolPtr symbol = FindSymbol(lhs);
    if (symbol == nullptr)
    {
      AddError(lhs->line, "unknown symbol '" + lhs->text + "'");
      return false;
    }
    if (symbol->IsVariable() == false)
    {
      // Type name or function name
      AddError(lhs->line, "operand does not support index operator");
      return false;
    }
    VariablePtr variable = ConvertToVariablePtr(symbol);
    if (variable->type == nullptr)
    {
      AddError(lhs->line, "variable '" + lhs->text + "' has unresolved type");
      return false;
    }
    SetVariableExpression(lhs, variable);
  }
  else
  {
    if (AnnotateExpression(lhs) == false)
    {
      return false;
    }
  }

  if (lhs->IsTypeExpression() || lhs->IsFunctionGroupExpression())
  {
    AddError(lhs->line, "operand does not support index operator");
    return false;
  }

  TypePtr type;
  if (lhs->type->IsInstantiation())
  {
    type = lhs->type->template_type;
  }
  else
  {
    type = lhs->type;
  }

  std::size_t const num_supplied_indexes = node->children.size() - 1;
  TypePtrArray      supplied_index_types;
  for (std::size_t i = 1; i <= num_supplied_indexes; ++i)
  {
    NodePtr const &   supplied_index      = node->children[i];
    ExpressionNodePtr supplied_index_node = ConvertToExpressionNodePtr(supplied_index);
    if (AnnotateExpression(supplied_index_node) == false)
    {
      return false;
    }
    supplied_index_types.push_back(supplied_index_node->type);
  }

  SymbolPtr symbol = type->symbols->Find(GET_INDEXED_VALUE);
  if (symbol == nullptr)
  {
    AddError(lhs->line,
             "unable to find matching index operator for type '" + lhs->type->name + "'");
    return false;
  }

  FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);

  TypePtrArray actual_index_types;
  FunctionPtr  f = FindFunction(lhs->type, fg, supplied_index_types, actual_index_types);
  if (f == nullptr)
  {
    AddError(lhs->line,
             "unable to find matching index operator for type '" + lhs->type->name + "'");
    return false;
  }

  for (std::size_t i = 1; i <= num_supplied_indexes; ++i)
  {
    NodePtr const &   supplied_index      = node->children[i];
    ExpressionNodePtr supplied_index_node = ConvertToExpressionNodePtr(supplied_index);
    supplied_index_node->type             = actual_index_types[i - 1];
  }

  TypePtr output_type = ConvertType(f->return_type, lhs->type);
  SetLVExpression(node, output_type);
  node->function = f;
  return true;
}

bool Analyser::AnnotateDotOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr  lhs         = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr  rhs         = ConvertToExpressionNodePtr(node->children[1]);
  std::string const &member_name = rhs->text;
  if ((lhs->node_kind == NodeKind::Identifier) || (lhs->node_kind == NodeKind::Template))
  {
    SymbolPtr symbol = FindSymbol(lhs);
    if (symbol == nullptr)
    {
      AddError(lhs->line, "unknown symbol '" + lhs->text + "'");
      return false;
    }
    if (symbol->IsVariable())
    {
      // Variable name
      VariablePtr variable = ConvertToVariablePtr(symbol);
      if (variable->type == nullptr)
      {
        AddError(lhs->line, "variable '" + lhs->text + "' has unresolved type");
        return false;
      }
      SetVariableExpression(lhs, variable);
    }
    else if (symbol->IsType())
    {
      // Type name
      TypePtr type = ConvertToTypePtr(symbol);
      SetTypeExpression(lhs, type);
    }
    else
    {
      // Function name
      AddError(lhs->line, "operand does not support member-access operator");
      return false;
    }
  }
  else
  {
    if (AnnotateExpression(lhs) == false)
    {
      return false;
    }
  }

  if (lhs->IsFunctionGroupExpression())
  {
    AddError(lhs->line, "operand does not support member-access operator");
    return false;
  }

  bool const lhs_is_instance =
      (lhs->IsVariableExpression()) || (lhs->IsLVExpression()) || (lhs->IsRVExpression());

  if (lhs->type->IsPrimitive())
  {
    AddError(lhs->line,
             "primitive type '" + lhs->type->name + "' does not support member-access operator");
    return false;
  }
  SymbolPtr member_symbol;
  if (lhs->type->IsInstantiation())
  {
    member_symbol = lhs->type->template_type->symbols->Find(member_name);
  }
  else
  {
    member_symbol = lhs->type->symbols->Find(member_name);
  }
  if (member_symbol == nullptr)
  {
    AddError(lhs->line, "type '" + lhs->type->name + "' has no member named '" + member_name + "'");
    return false;
  }
  if (member_symbol->IsFunctionGroup())
  {
    // member is a function name
    // static member function  lhs_is_instance == false
    // member function         lhs_is_instance == true
    FunctionGroupPtr fg = ConvertToFunctionGroupPtr(member_symbol);
    SetFunctionGroupExpression(node, fg, lhs->type, lhs_is_instance);
    return true;
  }
  else if (member_symbol->IsVariable())
  {
    // member is a variable name
    // static member variable  lhs_is_instance == false
    // member variable         lhs_is_instance == true
    AddError(lhs->line, "not supported");
    return false;
  }
  else
  {
    // member is a type name
    AddError(lhs->line, "not supported");
    return false;
  }
}

bool Analyser::AnnotateInvokeOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  if ((lhs->node_kind == NodeKind::Identifier) || (lhs->node_kind == NodeKind::Template))
  {
    SymbolPtr symbol = FindSymbol(lhs);
    if (symbol == nullptr)
    {
      AddError(lhs->line, "unknown symbol '" + lhs->text + "'");
      return false;
    }
    if (symbol->IsFunctionGroup())
    {
      // Function name
      FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
      SetFunctionGroupExpression(lhs, fg, nullptr, false);
    }
    else if (symbol->IsType())
    {
      // Type name
      TypePtr type = ConvertToTypePtr(symbol);
      SetTypeExpression(lhs, type);
    }
    else
    {
      // Variable name
      AddError(lhs->line, "operand does not support function-call operator");
      return false;
    }
  }
  else
  {
    if (AnnotateExpression(lhs) == false)
    {
      return false;
    }
  }

  TypePtrArray supplied_parameter_types;
  for (std::size_t i = 1; i < node->children.size(); ++i)
  {
    NodePtr const &   supplied_parameter      = node->children[i];
    ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
    if (AnnotateExpression(supplied_parameter_node) == false)
    {
      return false;
    }
    supplied_parameter_types.push_back(supplied_parameter_node->type);
  }

  if (lhs->IsFunctionGroupExpression())
  {
    // Opcode-invoked free function (lhs->type is nullptr)
    // Opcode-invoked static member function
    // Opcode-invoked member function
    // User-defined free function   (lhs->type is nullptr)
    TypePtrArray actual_parameter_types;
    FunctionPtr  f =
        FindFunction(lhs->type, lhs->fg, supplied_parameter_types, actual_parameter_types);
    if (f == nullptr)
    {
      // No matching function, or ambiguous
      AddError(lhs->line, "unable to find matching function for '" + lhs->fg->name + "'");
      return false;
    }

    if (f->function_kind == FunctionKind::StaticMemberFunction)
    {
      if (lhs->function_invoked_on_instance)
      {
        AddError(lhs->line, "function '" + lhs->fg->name + "' is a static member function");
        return false;
      }
    }
    else if (f->function_kind == FunctionKind::MemberFunction)
    {
      if (lhs->function_invoked_on_instance == false)
      {
        AddError(lhs->line, "function '" + lhs->fg->name + "' is a non-static member function");
        return false;
      }
    }

    for (std::size_t i = 1; i < node->children.size(); ++i)
    {
      NodePtr const &   supplied_parameter      = node->children[i];
      ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
      supplied_parameter_node->type             = actual_parameter_types[i - 1];
    }

    TypePtr return_type = ConvertType(f->return_type, lhs->type);
    SetRVExpression(node, return_type);
    node->function = f;
    return true;
  }
  else if (lhs->IsTypeExpression())
  {
    // Type constructor
    if (lhs->type->IsPrimitive())
    {
      AddError(lhs->line, "primitive type '" + lhs->type->name + "' is not constructible");
      return false;
    }
    SymbolPtr symbol;
    if (lhs->type->IsInstantiation())
    {
      symbol = lhs->type->template_type->symbols->Find(CONSTRUCTOR);
    }
    else
    {
      symbol = lhs->type->symbols->Find(CONSTRUCTOR);
    }
    if (symbol == nullptr)
    {
      AddError(lhs->line, "unable to find matching constructor for type '" + lhs->type->name + "'");
      return false;
    }
    FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
    TypePtrArray     actual_parameter_types;
    FunctionPtr f = FindFunction(lhs->type, fg, supplied_parameter_types, actual_parameter_types);
    if (f == nullptr)
    {
      // No matching constructor, or ambiguous
      AddError(lhs->line, "unable to find matching constructor for type '" + lhs->type->name + "'");
      return false;
    }

    for (std::size_t i = 1; i < node->children.size(); ++i)
    {
      NodePtr const &   supplied_parameter      = node->children[i];
      ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
      supplied_parameter_node->type             = actual_parameter_types[i - 1];
    }

    SetRVExpression(node, lhs->type);
    node->function = f;
    return true;
  }
  else
  {
    // e.g.
    // (a + b)();
    // array[index]();
    AddError(lhs->line, "operand does not support function-call operator");
    return false;
  }
}

// Returns true if control is able to reach the end of the block
bool Analyser::TestBlock(BlockNodePtr const &block_node)
{
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::ReturnStatement:
    case NodeKind::BreakStatement:
    case NodeKind::ContinueStatement:
    {
      return false;
    }
    case NodeKind::IfStatement:
    {
      bool const got_else = child->children.back()->node_kind == NodeKind::Else;
      if (got_else)
      {
        bool able_to_reach_end_of_if_statement = false;
        for (NodePtr const &nodeptr : child->children)
        {
          BlockNodePtr node                             = ConvertToBlockNodePtr(nodeptr);
          bool const   able_to_reach_reach_end_of_block = TestBlock(node);
          able_to_reach_end_of_if_statement |= able_to_reach_reach_end_of_block;
        }
        if (able_to_reach_end_of_if_statement == false)
        {
          return false;
        }
      }
      break;
    }
    case NodeKind::If:
    case NodeKind::ElseIf:
    case NodeKind::Else:
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
  bool const is_variable_expression = lhs->IsVariableExpression();
  bool const is_lv_expression       = lhs->IsLVExpression();
  if ((is_variable_expression == false) && (is_lv_expression == false))
  {
    AddError(lhs->line, "assignment operand is not writeable");
    return false;
  }
  if (is_variable_expression)
  {
    if ((lhs->variable->variable_kind == VariableKind::Parameter) ||
        (lhs->variable->variable_kind == VariableKind::For))
    {
      AddError(lhs->line, "assignment operand is not writeable");
      return false;
    }
  }
  return true;
}

bool Analyser::AnnotateArithmetic(ExpressionNodePtr const &node, ExpressionNodePtr const &lhs,
                                  ExpressionNodePtr const &rhs)
{
  Operator const op = GetOperator(node->node_kind);
  if (lhs->type->IsVoid() || lhs->type->IsNull())
  {
    AddError(node->line, "incompatible types");
    return false;
  }
  if (rhs->type->IsVoid() || rhs->type->IsNull())
  {
    AddError(node->line, "incompatible types");
    return false;
  }
  bool const lhs_is_primitive     = lhs->type->IsPrimitive();
  bool const rhs_is_primitive     = rhs->type->IsPrimitive();
  bool const lhs_is_instantiation = lhs->type->IsInstantiation();
  bool const rhs_is_instantiation = rhs->type->IsInstantiation();
  if (lhs_is_primitive)
  {
    if (rhs_is_primitive)
    {
      // primitive op primitive
      if ((lhs->type == rhs->type) && IsOperatorEnabled(lhs->type, op))
      {
        SetRVExpression(node, lhs->type);
        return true;
      }
    }
    else
    {
      // primitive op object
      if (rhs_is_instantiation && IsLeftOperatorEnabled(rhs->type, op))
      {
        TypePtr const &rhs_type = rhs->type->types[0];
        if (lhs->type == rhs_type)
        {
          SetRVExpression(node, rhs->type);
          return true;
        }
      }
    }
  }
  else
  {
    if (rhs_is_primitive)
    {
      // object op primitive
      if (lhs_is_instantiation && IsRightOperatorEnabled(lhs->type, op))
      {
        TypePtr const &lhs_type = lhs->type->types[0];
        if (lhs_type == rhs->type)
        {
          SetRVExpression(node, lhs->type);
          return true;
        }
      }
    }
    else
    {
      // object op object
      if ((lhs->type == rhs->type) && IsOperatorEnabled(lhs->type, op))
      {
        SetRVExpression(node, lhs->type);
        return true;
      }
    }
  }
  AddError(node->line, "incompatible types");
  return false;
}

TypePtr Analyser::ConvertType(TypePtr const &type, TypePtr const &instantiated_template_type)
{
  TypePtr converted_type;
  if (type->type_kind == TypeKind::Template)
  {
    converted_type = instantiated_template_type;
  }
  else if (type == template_parameter1_type_)
  {
    converted_type = instantiated_template_type->types[0];
  }
  else if (type == template_parameter2_type_)
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
  if (expected_type == any_type_)
  {
    return true;
  }
  else if (expected_type == any_primitive_type_)
  {
    return supplied_type->IsPrimitive();
  }
  else if (expected_type->IsGroup())
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
  std::size_t const num_types = expected_types.size();
  if (supplied_types.size() != num_types)
  {
    // Not a match
    return false;
  }
  for (std::size_t i = 0; i < num_types; ++i)
  {
    TypePtr const &supplied_type = supplied_types[i];
    if (supplied_type->IsVoid())
    {
      // Not a match
      return false;
    }
    TypePtr expected_type = ConvertType(expected_types[i], type);
    if (supplied_type->IsNull())
    {
      if (expected_type->IsPrimitive())
      {
        // Not a match
        return false;
      }
      actual_types.push_back(expected_type);
    }
    else
    {
      if (MatchType(supplied_type, expected_type) == false)
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
  // type is nullptr if fg is an opcode-invoked free function or a user-defined free function
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
  if (symbol->IsType() == false)
  {
    return nullptr;
  }
  return ConvertToTypePtr(symbol);
}

SymbolPtr Analyser::FindSymbol(ExpressionNodePtr const &node)
{
  SymbolPtr symbol;
  if (node->node_kind == NodeKind::Template)
  {
    symbol = SearchSymbols(node->text);
    if (symbol)
    {
      // Template instantiation already exists
      return ConvertToTypePtr(symbol);
    }
    ExpressionNodePtr  identifier_node = ConvertToExpressionNodePtr(node->children[0]);
    std::string const &name            = identifier_node->text;
    symbol                             = SearchSymbols(name);
    if (symbol == nullptr)
    {
      // Template type doesn't exist
      return nullptr;
    }
    TypePtr           template_type                = ConvertToTypePtr(symbol);
    std::size_t const num_supplied_parameter_types = node->children.size() - 1;
    std::size_t const num_expected_parameter_types = template_type->types.size();
    TypePtrArray      parameter_types;
    if (num_supplied_parameter_types != num_expected_parameter_types)
    {
      return nullptr;
    }
    for (std::size_t i = 1; i <= num_expected_parameter_types; ++i)
    {
      ExpressionNodePtr parameter_type_node = ConvertToExpressionNodePtr(node->children[i]);
      TypePtr           parameter_type      = FindType(parameter_type_node);
      if (parameter_type == nullptr)
      {
        return nullptr;
      }
      TypePtr const &expected_parameter_type = template_type->types[i - 1];
      if (MatchType(parameter_type, expected_parameter_type) == false)
      {
        return nullptr;
      }
      // Need to check here that parameter_type does in fact support any operator(s)
      // required by the template_type's i'th type parameter...
      parameter_types.push_back(parameter_type);
    }
    TypePtr type = InternalCreateInstantiationType(TypeKind::UserDefinedInstantiation,
                                                   template_type, parameter_types);
    root_->symbols->Add(type);
    return type;
  }
  else  // (node->node_kind == NodeKind::Identifier)
  {
    std::string const &name = node->text;
    symbol                  = SearchSymbols(name);
    if (symbol)
    {
      return symbol;
    }
    return nullptr;
  }
}

SymbolPtr Analyser::SearchSymbols(std::string const &name)
{
  SymbolPtr symbol = symbols_->Find(name);
  if (symbol)
  {
    return symbol;
  }
  auto it  = blocks_.rbegin();
  auto end = blocks_.rend();
  while (it != end)
  {
    BlockNodePtr const &block_node = *it;
    symbol                         = block_node->symbols->Find(name);
    if (symbol)
    {
      return symbol;
    }
    ++it;
  }
  // Name not found in any symbol table
  return nullptr;
}

void Analyser::SetVariableExpression(ExpressionNodePtr const &node, VariablePtr const &variable)
{
  node->expression_kind = ExpressionKind::Variable;
  node->variable        = variable;
  node->type            = variable->type;
}

void Analyser::SetLVExpression(ExpressionNodePtr const &node, TypePtr const &type)
{
  node->expression_kind = ExpressionKind::LV;
  node->type            = type;
}

void Analyser::SetRVExpression(ExpressionNodePtr const &node, TypePtr const &type)
{
  node->expression_kind = ExpressionKind::RV;
  node->type            = type;
}

void Analyser::SetTypeExpression(ExpressionNodePtr const &node, TypePtr const &type)
{
  node->expression_kind = ExpressionKind::Type;
  node->type            = type;
}

void Analyser::SetFunctionGroupExpression(ExpressionNodePtr const &node, FunctionGroupPtr const &fg,
                                          TypePtr const &fg_owner,
                                          bool           function_invoked_on_instance)
{
  node->expression_kind              = ExpressionKind::FunctionGroup;
  node->type                         = fg_owner;
  node->fg                           = fg;
  node->function_invoked_on_instance = function_invoked_on_instance;
}

void Analyser::CreatePrimitiveType(std::string const &type_name, TypeIndex type_index,
                                   bool add_to_symbol_table, TypeId type_id, TypePtr &type)
{
  if (type_map_.Find(type_index))
  {
    // Already created
    return;
  }
  type = CreateType(TypeKind::Primitive, type_name);
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeInfo(TypeKind::Primitive, type_name, {}), type_id, type);
  registered_types_.Add(type_index, type->id);
  if (add_to_symbol_table)
  {
    symbols_->Add(type);
  }
}

void Analyser::CreateMetaType(std::string const &type_name, TypeIndex type_index, TypeId type_id,
                              TypePtr &type)
{
  if (type_map_.Find(type_index))
  {
    // Already created
    return;
  }
  type = CreateType(TypeKind::Meta, type_name);
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeInfo(TypeKind::Meta, type_name, {}), type_id, type);
  registered_types_.Add(type_index, type->id);
}

void Analyser::CreateClassType(std::string const &type_name, TypeIndex type_index, TypeId type_id,
                               TypePtr &type)
{
  if (type_map_.Find(type_index))
  {
    // Already created
    return;
  }
  type          = CreateType(TypeKind::Class, type_name);
  type->symbols = CreateSymbolTable();
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeInfo(TypeKind::Class, type_name, {}), type_id, type);
  registered_types_.Add(type_index, type->id);
  symbols_->Add(type);
}

void Analyser::CreateTemplateType(std::string const &type_name, TypeIndex type_index,
                                  TypePtrArray const &allowed_types, TypeId type_id, TypePtr &type)
{
  if (type_map_.Find(type_index))
  {
    // Already created
    return;
  }
  type          = CreateType(TypeKind::Template, type_name);
  type->symbols = CreateSymbolTable();
  type->types   = allowed_types;
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeInfo(TypeKind::Template, type_name, {}), type_id, type);
  registered_types_.Add(type_index, type->id);
  symbols_->Add(type);
}

void Analyser::CreateInstantiationType(TypeIndex type_index, TypePtr const &template_type,
                                       TypePtrArray const &parameter_types, TypeId type_id,
                                       TypePtr &type)
{
  if (type_map_.Find(type_index))
  {
    // Already created
    return;
  }
  type = InternalCreateInstantiationType(TypeKind::Instantiation, template_type, parameter_types);
  TypeIdArray parameter_type_ids;
  for (auto const &parameter_type : parameter_types)
  {
    parameter_type_ids.push_back(parameter_type->id);
  }
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeInfo(TypeKind::Instantiation, type->name, parameter_type_ids), type_id, type);
  registered_types_.Add(type_index, type->id);
  symbols_->Add(type);
}

void Analyser::CreateGroupType(std::string const &type_name, TypeIndex type_index,
                               TypePtrArray const &allowed_types, TypeId type_id, TypePtr &type)
{
  if (type_map_.Find(type_index))
  {
    // Already created
    return;
  }
  type        = CreateType(TypeKind::Group, type_name);
  type->types = allowed_types;
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeInfo(TypeKind::Group, type_name, {}), type_id, type);
  registered_types_.Add(type_index, type->id);
}

TypePtr Analyser::InternalCreateInstantiationType(TypeKind type_kind, TypePtr const &template_type,
                                                  TypePtrArray const &parameter_types)
{
  std::stringstream stream;
  stream << template_type->name + "<";
  std::size_t const count = parameter_types.size();
  for (std::size_t i = 0; i < count; ++i)
  {
    stream << parameter_types[i]->name;
    if (i + 1 < count)
    {
      stream << ",";
    }
  }
  stream << ">";
  std::string name    = stream.str();
  TypePtr     type    = CreateType(type_kind, name);
  type->template_type = template_type;
  type->types         = parameter_types;
  return type;
}

void Analyser::CreateFreeFunction(std::string const &name, TypePtrArray const &parameter_types,
                                  TypePtr const &return_type, Handler const &handler)
{
  std::string unique_id = BuildUniqueId(nullptr, name, parameter_types, return_type);
  FunctionPtr f = CreateFunction(FunctionKind::FreeFunction, name, unique_id, parameter_types,
                                 VariablePtrArray(), return_type);
  AddFunctionToSymbolTable(symbols_, f);
  AddFunctionInfo(f, handler);
}

void Analyser::CreateConstructor(TypePtr const &type, TypePtrArray const &parameter_types,
                                 Handler const &handler)
{
  std::string unique_id = BuildUniqueId(type, CONSTRUCTOR, parameter_types, type);
  FunctionPtr f = CreateFunction(FunctionKind::Constructor, CONSTRUCTOR, unique_id, parameter_types,
                                 VariablePtrArray(), type);
  AddFunctionToSymbolTable(type->symbols, f);
  AddFunctionInfo(f, handler);
}

void Analyser::CreateStaticMemberFunction(TypePtr const &type, std::string const &name,
                                          TypePtrArray const &parameter_types,
                                          TypePtr const &return_type, Handler const &handler)
{
  std::string unique_id = BuildUniqueId(type, name, parameter_types, return_type);
  FunctionPtr f         = CreateFunction(FunctionKind::StaticMemberFunction, name, unique_id,
                                 parameter_types, VariablePtrArray(), return_type);
  AddFunctionToSymbolTable(type->symbols, f);
  AddFunctionInfo(f, handler);
}

void Analyser::CreateMemberFunction(TypePtr const &type, std::string const &name,
                                    TypePtrArray const &parameter_types, TypePtr const &return_type,
                                    Handler const &handler)
{
  std::string unique_id = BuildUniqueId(type, name, parameter_types, return_type);
  FunctionPtr f = CreateFunction(FunctionKind::MemberFunction, name, unique_id, parameter_types,
                                 VariablePtrArray(), return_type);
  AddFunctionToSymbolTable(type->symbols, f);
  AddFunctionInfo(f, handler);
}

FunctionPtr Analyser::CreateUserDefinedFreeFunction(std::string const &     name,
                                                    TypePtrArray const &    parameter_types,
                                                    VariablePtrArray const &parameter_variables,
                                                    TypePtr const &         return_type)
{
  return CreateFunction(FunctionKind::UserDefinedFreeFunction, name, name, parameter_types,
                        parameter_variables, return_type);
}

void Analyser::EnableIndexOperator(TypePtr const &type, TypePtrArray const &input_types,
                                   TypePtr const &output_type, Handler const &get_handler,
                                   Handler const &set_handler)
{
  std::string g_unique_id = BuildUniqueId(type, GET_INDEXED_VALUE, input_types, output_type);
  FunctionPtr gf = CreateFunction(FunctionKind::MemberFunction, GET_INDEXED_VALUE, g_unique_id,
                                  input_types, VariablePtrArray(), output_type);
  AddFunctionInfo(gf, get_handler);
  AddFunctionToSymbolTable(type->symbols, gf);

  TypePtrArray s_input_types = input_types;
  s_input_types.push_back(output_type);
  std::string s_unique_id = BuildUniqueId(type, SET_INDEXED_VALUE, s_input_types, void_type_);
  FunctionPtr sf = CreateFunction(FunctionKind::MemberFunction, SET_INDEXED_VALUE, s_unique_id,
                                  s_input_types, VariablePtrArray(), void_type_);
  AddFunctionInfo(sf, set_handler);
  AddFunctionToSymbolTable(type->symbols, sf);
}

void Analyser::AddTypeInfo(TypeInfo const &info, TypeId type_id, TypePtr const &type)
{
  TypeId id;
  if (type_id == TypeIds::Unknown)
  {
    id = TypeId(type_info_array_.size());
    type_info_array_.push_back(info);
  }
  else
  {
    id                   = type_id;
    type_info_array_[id] = info;
  }
  type->id                   = id;
  type_info_map_[type->name] = id;
}

void Analyser::AddFunctionInfo(FunctionPtr const &function, Handler const &handler)
{
  FunctionInfo info(function->function_kind, function->unique_id, handler);
  function_info_array_.push_back(info);
}

std::string Analyser::BuildUniqueId(TypePtr const &type, std::string const &function_name,
                                    TypePtrArray const &parameter_types, TypePtr const &return_type)
{
  std::stringstream stream;
  if (type)
  {
    stream << type->name;
  }
  stream << "::" << function_name << "^";
  std::size_t const count = parameter_types.size();
  for (std::size_t i = 0; i < count; ++i)
  {
    stream << parameter_types[i]->name;
    if (i + 1 < count)
    {
      stream << ",";
    }
  }
  stream << "^" << return_type->name;
  std::string unique_id = stream.str();
  return unique_id;
}

void Analyser::AddFunctionToSymbolTable(SymbolTablePtr const &symbols, FunctionPtr const &function)
{
  FunctionGroupPtr fg;
  SymbolPtr        symbol = symbols->Find(function->name);
  if (symbol)
  {
    fg = ConvertToFunctionGroupPtr(symbol);
  }
  else
  {
    // Create new function group
    fg = CreateFunctionGroup(function->name);
    symbols->Add(fg);
  }
  // Add the function to the function group
  fg->functions.push_back(function);
}

}  // namespace vm
}  // namespace fetch
