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

#include "vm/analyser.hpp"
#include "vm/module.hpp"
#include <sstream>

namespace fetch {
namespace vm {

std::string Analyser::CONSTRUCTOR = "$constructor$";

Analyser::Analyser(Module *module)
{
  symbols_ = CreateSymbolTable();

  template_instantiation_type_ =
      CreateType("TemplateInstantiation", Type::Category::Primitive, TypeId::TemplateInstantiation);
  template_parameter_type1_ =
      CreateType("TemplateParameter1", Type::Category::Primitive, TypeId::TemplateParameter1);
  template_parameter_type2_ =
      CreateType("TemplateParameter2", Type::Category::Primitive, TypeId::TemplateParameter2);
  numeric_type_ = CreateType("Numeric", Type::Category::Primitive, TypeId::Numeric);

  matrix_template_type_ = CreateTemplateType("Matrix", TypeId::MatrixTemplate);
  array_template_type_  = CreateTemplateType("Array", TypeId::ArrayTemplate);

  // Defines a new type 
  void_type_ = CreateType("Void", Type::Category::Primitive, TypeId::Void);
  // ... and make a mapping to its corresponding C++ type
  RegisterType<void>(void_type_);

  null_type_ = CreateType("Null", Type::Category::Primitive, TypeId::Null);
  RegisterType<std::nullptr_t>(null_type_);

  bool_type_ = CreatePrimitiveType("Bool", TypeId::Bool);
  RegisterType<bool>(bool_type_);

  int8_type_ = CreatePrimitiveType("Int8", TypeId::Int8);
  RegisterType<int8_t>(int8_type_);

  byte_type_ = CreatePrimitiveType("Byte", TypeId::Byte);
  RegisterType<uint8_t>(byte_type_);

  int16_type_ = CreatePrimitiveType("Int16", TypeId::Int16);
  RegisterType<int16_t>(int16_type_);

  uint16_type_ = CreatePrimitiveType("UInt16", TypeId::UInt16);
  RegisterType<uint16_t>(uint16_type_);

  int32_type_ = CreatePrimitiveType("Int32", TypeId::Int32);
  RegisterType<int32_t>(int32_type_);

  uint32_type_ = CreatePrimitiveType("UInt32", TypeId::UInt32);
  RegisterType<uint32_t>(uint32_type_);

  int64_type_ = CreatePrimitiveType("Int64", TypeId::Int64);
  RegisterType<int64_t>(int64_type_);

  uint64_type_ = CreatePrimitiveType("UInt64", TypeId::UInt64);
  RegisterType<uint64_t>(uint64_type_);

  float32_type_ = CreatePrimitiveType("Float32", TypeId::Float32);
  RegisterType<float>(float32_type_);

  float64_type_ = CreatePrimitiveType("Float64", TypeId::Float64);
  RegisterType<double>(float64_type_);


  // Non-primitive builtin types
  // Note these are not registered yet, as custom modules do not have support for 
  // templating just yet.
  string_type_         = CreateClassType("String", TypeId::String);
  matrix_float32_type_ = CreateTemplateInstantiationType("Matrix<Float32>", TypeId::Matrix_Float32,
                                                         matrix_template_type_, {float32_type_});
  matrix_float64_type_ = CreateTemplateInstantiationType("Matrix<Float64>", TypeId::Matrix_Float64,
                                                         matrix_template_type_, {float64_type_});
  array_bool_type_     = CreateTemplateInstantiationType("Array<Bool>", TypeId::Array_Bool,
                                                     array_template_type_, {bool_type_});
  array_int8_type_     = CreateTemplateInstantiationType("Array<Int8>", TypeId::Array_Int8,
                                                     array_template_type_, {int8_type_});
  array_byte_type_     = CreateTemplateInstantiationType("Array<Byte>", TypeId::Array_Byte,
                                                     array_template_type_, {byte_type_});
  array_int16_type_    = CreateTemplateInstantiationType("Array<Int16>", TypeId::Array_Int16,
                                                      array_template_type_, {int16_type_});
  array_uint16_type_   = CreateTemplateInstantiationType("Array<UInt16>", TypeId::Array_UInt16,
                                                       array_template_type_, {uint16_type_});
  array_int32_type_    = CreateTemplateInstantiationType("Array<Int32>", TypeId::Array_Int32,
                                                      array_template_type_, {int32_type_});
  array_uint32_type_   = CreateTemplateInstantiationType("Array<UInt32>", TypeId::Array_UInt32,
                                                       array_template_type_, {uint32_type_});
  array_int64_type_    = CreateTemplateInstantiationType("Array<Int64>", TypeId::Array_Int64,
                                                      array_template_type_, {int64_type_});
  array_uint64_type_   = CreateTemplateInstantiationType("Array<UInt64>", TypeId::Array_UInt64,
                                                       array_template_type_, {uint64_type_});
  array_float32_type_  = CreateTemplateInstantiationType("Array<Float32>", TypeId::Array_Float32,
                                                        array_template_type_, {float32_type_});
  array_float64_type_  = CreateTemplateInstantiationType("Array<Float64>", TypeId::Array_Float64,
                                                        array_template_type_, {float64_type_});
  array_string_type_   = CreateTemplateInstantiationType("Array<String>", TypeId::Array_String,
                                                       array_template_type_, {string_type_});
  array_matrix_float32_type_ =
      CreateTemplateInstantiationType("Array<Matrix<Float32>>", TypeId::Array_Matrix_Float32,
                                      array_template_type_, {matrix_float32_type_});
  array_matrix_float64_type_ =
      CreateTemplateInstantiationType("Array<Matrix<Float64>>", TypeId::Array_Matrix_Float64,
                                      array_template_type_, {matrix_float64_type_});

  // Casts
  CreateOpcodeFunction("toInt8", {numeric_type_}, int8_type_, Opcode::ToInt8);
  CreateOpcodeFunction("toByte", {numeric_type_}, byte_type_, Opcode::ToByte);
  CreateOpcodeFunction("toInt16", {numeric_type_}, int16_type_, Opcode::ToInt16);
  CreateOpcodeFunction("toUInt16", {numeric_type_}, uint16_type_, Opcode::ToUInt16);
  CreateOpcodeFunction("toInt32", {numeric_type_}, int32_type_, Opcode::ToInt32);
  CreateOpcodeFunction("toUInt32", {numeric_type_}, uint32_type_, Opcode::ToUInt32);
  CreateOpcodeFunction("toInt64", {numeric_type_}, int64_type_, Opcode::ToInt64);
  CreateOpcodeFunction("toUInt64", {numeric_type_}, uint64_type_, Opcode::ToUInt64);
  CreateOpcodeFunction("toFloat32", {numeric_type_}, float32_type_, Opcode::ToFloat32);
  CreateOpcodeFunction("toFloat64", {numeric_type_}, float64_type_, Opcode::ToFloat64);

  CreateOpcodeTypeFunction(matrix_template_type_, CONSTRUCTOR, {int32_type_, int32_type_},
                           template_instantiation_type_, Opcode::CreateMatrix);
  CreateOpcodeTypeFunction(array_template_type_, CONSTRUCTOR, {int32_type_},
                           template_instantiation_type_, Opcode::CreateArray);

  // Custom functions


  // If new additional functionality is defined in the module
  // we set the appropriate opcodes up here.
  if (module != nullptr)
  {
    module->Setup(this);
  }
}

bool Analyser::Analyse(const BlockNodePtr &root, std::vector<std::string> &errors)
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

  if (errors_.size() != 0)
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

  if (errors_.size() != 0)
  {
    errors = std::move(errors_);
    root_  = nullptr;
    blocks_.clear();
    loops_.clear();
    function_ = nullptr;
    errors_.clear();
    return false;
  }

  root_ = nullptr;
  blocks_.clear();
  loops_.clear();
  function_ = nullptr;
  return true;
}

void Analyser::AddError(const Token &token, const std::string &message)
{
  std::stringstream stream;
  stream << "line " << token.line << ": "
         << "error: " << message;
  errors_.push_back(stream.str());
}

void Analyser::BuildBlock(const BlockNodePtr &block_node)
{
  for (const NodePtr &child : block_node->block_children)
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
      BuildWhileStatement(block_node, ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::ForStatement:
    {
      BuildForStatement(block_node, ConvertToBlockNodePtr(child));
      break;
    }
    case Node::Kind::IfStatement:
    {
      BuildIfStatement(block_node, child);
      break;
    }
    default:
      break;
    }
  }
}

void Analyser::BuildFunctionDefinition(const BlockNodePtr &parent_block_node,
                                       const BlockNodePtr &function_definition_node)
{
  function_definition_node->symbols = CreateSymbolTable();
  ExpressionNodePtr identifier_node =
      ConvertToExpressionNodePtr(function_definition_node->children[0]);
  const std::string &      name  = identifier_node->token.text;
  const int                count = (int)function_definition_node->children.size();
  std::vector<VariablePtr> parameter_variables;
  std::vector<TypePtr>     parameter_types;
  const int                num_parameters = int((count - 1) / 2);
  int                      problems       = 0;
  for (int i = 0; i < num_parameters; ++i)
  {
    ExpressionNodePtr parameter_node =
        ConvertToExpressionNodePtr(function_definition_node->children[std::size_t(1 + i * 2)]);
    const std::string &parameter_name = parameter_node->token.text;
    SymbolPtr          symbol         = function_definition_node->symbols->Find(parameter_name);
    if (symbol)
    {
      std::stringstream stream;
      AddError(parameter_node->token, "parameter name '" + parameter_name + "' is already defined");
      ++problems;
      continue;
    }
    ExpressionNodePtr parameter_type_node =
        ConvertToExpressionNodePtr(function_definition_node->children[std::size_t(2 + i * 2)]);
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
    function_definition_node->symbols->Add(parameter_name, parameter_variable);
    parameter_node->variable = parameter_variable;
    parameter_node->type     = parameter_variable->type;
    parameter_variables.push_back(parameter_variable);
    parameter_types.push_back(parameter_type);
  }
  const bool return_type_supplied = (count % 2 == 0);
  TypePtr    return_type;
  if (return_type_supplied)
  {
    ExpressionNodePtr return_type_node =
        ConvertToExpressionNodePtr(function_definition_node->children[std::size_t(count - 1)]);
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
  if (problems) return;
  FunctionGroupPtr fg;
  SymbolPtr        symbol = parent_block_node->symbols->Find(name);
  if (symbol)
  {
    fg = ConvertToFunctionGroupPtr(symbol);
    std::vector<TypePtr> output_parameter_types;
    if (FindMatchingFunction(fg, nullptr, parameter_types, output_parameter_types))
    {
      AddError(function_definition_node->token,
               "function '" + name + "' is already defined with the same parameter types");
      return;
    }
  }
  else
  {
    fg = CreateFunctionGroup(name);
    parent_block_node->symbols->Add(name, fg);
  }
  FunctionPtr function =
      CreateUserFunction(name, parameter_types, parameter_variables, return_type);
  fg->functions.push_back(function);
  identifier_node->function = function;
  BuildBlock(function_definition_node);
}

void Analyser::BuildWhileStatement(const BlockNodePtr &parent_block_node,
                                   const BlockNodePtr &while_statement_node)
{
  while_statement_node->symbols = CreateSymbolTable();
  BuildBlock(while_statement_node);
}

void Analyser::BuildForStatement(const BlockNodePtr &parent_block_node,
                                 const BlockNodePtr &for_statement_node)
{
  for_statement_node->symbols = CreateSymbolTable();
  BuildBlock(for_statement_node);
}

void Analyser::BuildIfStatement(const BlockNodePtr &parent_block_node,
                                const NodePtr &     if_statement_node)
{
  for (const NodePtr &child : if_statement_node->children)
  {
    BlockNodePtr block_node = ConvertToBlockNodePtr(child);
    block_node->symbols     = CreateSymbolTable();
    BuildBlock(block_node);
  }
}

void Analyser::AnnotateBlock(const BlockNodePtr &block_node)
{
  blocks_.push_back(block_node);
  const bool is_loop = ((block_node->kind == Node::Kind::WhileStatement) ||
                        (block_node->kind == Node::Kind::ForStatement));
  if (is_loop) loops_.push_back(block_node);
  for (const NodePtr &child : block_node->block_children)
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
        AddError(child->token, "break statement is not inside a while or for loop");
      break;
    }
    case Node::Kind::ContinueStatement:
    {
      if (loops_.size() == 0)
        AddError(child->token, "continue statement is not inside a while or for loop");
      break;
    }
    case Node::Kind::AssignOp:
    {
      AnnotateAssignOp(ConvertToExpressionNodePtr(child));
      break;
    }
    case Node::Kind::AddAssignOp:
    case Node::Kind::SubtractAssignOp:
    {
      AnnotateAddSubtractAssignOp(ConvertToExpressionNodePtr(child));
      break;
    }
    case Node::Kind::MultiplyAssignOp:
    {
      AnnotateMultiplyAssignOp(ConvertToExpressionNodePtr(child));
      break;
    }
    case Node::Kind::DivideAssignOp:
    {
      AnnotateDivideAssignOp(ConvertToExpressionNodePtr(child));
      break;
    }
    default:
    {
      AnnotateExpression(ConvertToExpressionNodePtr(child));
      break;
    }
    }
  }
  if (is_loop) loops_.pop_back();
  blocks_.pop_back();
}

void Analyser::AnnotateFunctionDefinitionStatement(const BlockNodePtr &function_definition_node)
{
  ExpressionNodePtr identifier_node =
      ConvertToExpressionNodePtr(function_definition_node->children[0]);
  function_ = identifier_node->function;
  AnnotateBlock(function_definition_node);
  if (errors_.size() == 0)
  {
    if (function_->return_type->id != TypeId::Void)
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

// Returns true if control is able to reach the end of the block
bool Analyser::TestBlock(const BlockNodePtr &block_node)
{
  for (const NodePtr &child : block_node->block_children)
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
      const bool got_else = child->children.back()->kind == Node::Kind::Else;
      if (got_else)
      {
        bool able_to_reach_end_of_if_statement = false;
        for (const NodePtr &nodeptr : child->children)
        {
          BlockNodePtr node                             = ConvertToBlockNodePtr(nodeptr);
          const bool   able_to_reach_reach_end_of_block = TestBlock(node);
          able_to_reach_end_of_if_statement |= able_to_reach_reach_end_of_block;
        }
        if (able_to_reach_end_of_if_statement == false) return false;
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
      break;
    }
  }
  // No child returned false, so we are indeed able to reach the end of the block
  return true;
}

void Analyser::AnnotateWhileStatement(const BlockNodePtr &while_statement_node)
{
  AnnotateConditionalBlock(while_statement_node);
}

void Analyser::AnnotateForStatement(const BlockNodePtr &for_statement_node)
{
  ExpressionNodePtr  identifier_node = ConvertToExpressionNodePtr(for_statement_node->children[0]);
  const std::string &name            = identifier_node->token.text;
  // Note: variable is created with no type
  VariablePtr variable = CreateVariable(name, Variable::Category::For);
  for_statement_node->symbols->Add(name, variable);
  identifier_node->variable            = variable;
  const int                      count = (int)for_statement_node->children.size() - 1;
  std::vector<ExpressionNodePtr> nodes;
  int                            problems = 0;
  for (std::size_t i = 1; i <= std::size_t(count); ++i)
  {
    const NodePtr &   child      = for_statement_node->children[i];
    ExpressionNodePtr child_node = ConvertToExpressionNodePtr(child);
    if (AnnotateExpression(child_node) == false)
    {
      ++problems;
      continue;
    }
    if (IsIntegralType(child_node->type->id) == false)
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

void Analyser::AnnotateIfStatement(const NodePtr &if_statement_node)
{
  for (const NodePtr &child : if_statement_node->children)
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

void Analyser::AnnotateVarStatement(const BlockNodePtr &parent_block_node,
                                    const NodePtr &     var_statement_node)
{
  ExpressionNodePtr  identifier_node = ConvertToExpressionNodePtr(var_statement_node->children[0]);
  const std::string &name            = identifier_node->token.text;
  SymbolPtr          symbol          = parent_block_node->symbols->Find(name);
  if (symbol)
  {
    AddError(identifier_node->token, "variable '" + name + "' is already defined");
    return;
  }
  // Note: variable is created with no type
  VariablePtr variable = CreateVariable(name, Variable::Category::Local);
  parent_block_node->symbols->Add(name, variable);
  identifier_node->variable = variable;
  if (var_statement_node->kind == Node::Kind::VarDeclarationStatement)
  {
    ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (AnnotateTypeExpression(type_node) == false) return;
    variable->type = type_node->type;
  }
  else if (var_statement_node->kind == Node::Kind::VarDeclarationTypedAssignmentStatement)
  {
    ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (AnnotateTypeExpression(type_node) == false) return;
    ExpressionNodePtr expression_node = ConvertToExpressionNodePtr(var_statement_node->children[2]);
    if (AnnotateExpression(expression_node) == false) return;
    if (expression_node->type->id != TypeId::Null)
    {
      if (type_node->type != expression_node->type)
      {
        AddError(type_node->token, "incompatible types");
        return;
      }
    }
    else
    {
      if (type_node->type->IsPrimitiveType())
      {
        // can't assign null to a primitive type
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
    if (AnnotateExpression(expression_node) == false) return;
    if ((expression_node->type->id == TypeId::Void) || (expression_node->type->id == TypeId::Null))
    {
      AddError(expression_node->token, "unable to infer type");
      return;
    }
    variable->type = expression_node->type;
  }
}

void Analyser::AnnotateReturnStatement(const NodePtr &return_statement_node)
{
  if (return_statement_node->children.size() == 1)
  {
    ExpressionNodePtr expression_node =
        ConvertToExpressionNodePtr(return_statement_node->children[0]);
    if (AnnotateExpression(expression_node) == false) return;
    if (expression_node->type->id != TypeId::Null)
    {
      if (expression_node->type != function_->return_type)
      {
        AddError(expression_node->token, "type does not match function return type");
        return;
      }
    }
    else
    {
      if (function_->return_type->IsPrimitiveType())
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
    if (function_->return_type->id != TypeId::Void)
    {
      AddError(return_statement_node->token, "return does not supply a value");
      return;
    }
  }
}

void Analyser::AnnotateConditionalBlock(const BlockNodePtr &conditional_node)
{
  ExpressionNodePtr expression_node = ConvertToExpressionNodePtr(conditional_node->children[0]);
  if (AnnotateExpression(expression_node))
  {
    if (expression_node->type->id != TypeId::Bool)
    {
      AddError(expression_node->token, "boolean type expected");
    }
  }
  AnnotateBlock(conditional_node);
}

bool Analyser::AnnotateTypeExpression(const ExpressionNodePtr &node)
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

bool Analyser::AnnotateAssignOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (IsWriteable(lhs) == false) return false;
  if (rhs->type->id == TypeId::Void)
  {
    // can't assign from a Void-returning function
    AddError(node->token, "incompatible types");
    return false;
  }
  if (rhs->type->id != TypeId::Null)
  {
    if (lhs->type != rhs->type)
    {
      AddError(node->token, "incompatible types");
      return false;
    }
  }
  else
  {
    if (lhs->type->IsPrimitiveType())
    {
      // can't assign null to a primitive type
      AddError(node->token, "incompatible types");
      return false;
    }
    // Convert the null type to the correct type
    rhs->type = lhs->type;
  }
  SetRV(node, lhs->type);
  return true;
}

bool Analyser::AnnotateAddSubtractAssignOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (IsWriteable(lhs) == false) return false;
  TypePtr type = IsAddSubtractCompatible(node, lhs, rhs);
  if (type == nullptr) return false;
  SetRV(node, type);
  return true;
}

bool Analyser::AnnotateMultiplyAssignOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (IsWriteable(lhs) == false) return false;
  TypePtr type = IsMultiplyCompatible(node, lhs, rhs);
  if (type == nullptr) return false;
  SetRV(node, type);
  return true;
}

bool Analyser::AnnotateDivideAssignOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (IsWriteable(lhs) == false) return false;
  if (IsDivideCompatible(node, lhs, rhs) == nullptr) return false;
  SetRV(node, lhs->type);
  return true;
}

bool Analyser::IsWriteable(const ExpressionNodePtr &lhs)
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

TypePtr Analyser::IsAddSubtractCompatible(const ExpressionNodePtr &node,
                                          const ExpressionNodePtr &lhs,
                                          const ExpressionNodePtr &rhs)
{
  const bool lhs_numeric = IsNumericType(lhs->type->id);
  const bool rhs_numeric = IsNumericType(rhs->type->id);
  const bool lhs_matrix  = IsMatrixType(lhs->type->id);
  const bool rhs_matrix  = IsMatrixType(rhs->type->id);
  if (lhs_numeric && rhs_numeric)
  {
    if (lhs->type != rhs->type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return lhs->type;
  }
  else if (lhs_matrix && rhs_matrix)
  {
    const TypePtr &lhs_instantiation_type = lhs->type->template_parameter_types[0];
    const TypePtr &rhs_instantiation_type = rhs->type->template_parameter_types[0];
    if (lhs_instantiation_type != rhs_instantiation_type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return lhs->type;
  }
  else if (lhs_numeric && rhs_matrix)
  {
    AddError(node->token, "incompatible types");
    return nullptr;
  }
  else if (lhs_matrix && rhs_numeric)
  {
    const TypePtr &lhs_instantiation_type = lhs->type->template_parameter_types[0];
    if (lhs_instantiation_type != rhs->type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return lhs->type;
  }
  else
  {
    AddError(node->token, "incompatible types");
    return nullptr;
  }
}

TypePtr Analyser::IsMultiplyCompatible(const ExpressionNodePtr &node, const ExpressionNodePtr &lhs,
                                       const ExpressionNodePtr &rhs)
{
  const bool lhs_numeric = IsNumericType(lhs->type->id);
  const bool rhs_numeric = IsNumericType(rhs->type->id);
  const bool lhs_matrix  = IsMatrixType(lhs->type->id);
  const bool rhs_matrix  = IsMatrixType(rhs->type->id);
  if (lhs_numeric && rhs_numeric)
  {
    if (lhs->type != rhs->type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return lhs->type;
  }
  else if (lhs_matrix && rhs_matrix)
  {
    const TypePtr &lhs_instantiation_type = lhs->type->template_parameter_types[0];
    const TypePtr &rhs_instantiation_type = rhs->type->template_parameter_types[0];
    if (lhs_instantiation_type != rhs_instantiation_type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return lhs->type;
  }
  else if (lhs_numeric && rhs_matrix)
  {
    const TypePtr &rhs_instantiation_type = rhs->type->template_parameter_types[0];
    if (lhs->type != rhs_instantiation_type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return rhs->type;
  }
  else if (lhs_matrix && rhs_numeric)
  {
    const TypePtr &lhs_instantiation_type = lhs->type->template_parameter_types[0];
    if (lhs_instantiation_type != rhs->type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return lhs->type;
  }
  else
  {
    AddError(node->token, "incompatible types");
    return nullptr;
  }
}

TypePtr Analyser::IsDivideCompatible(const ExpressionNodePtr &node, const ExpressionNodePtr &lhs,
                                     const ExpressionNodePtr &rhs)
{
  const bool lhs_numeric = IsNumericType(lhs->type->id);
  const bool rhs_numeric = IsNumericType(rhs->type->id);
  const bool lhs_matrix  = IsMatrixType(lhs->type->id);
  const bool rhs_matrix  = IsMatrixType(rhs->type->id);
  if (lhs_numeric && rhs_numeric)
  {
    if (lhs->type != rhs->type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return lhs->type;
  }
  else if (lhs_matrix && rhs_matrix)
  {
    AddError(node->token, "incompatible types");
    return nullptr;
  }
  else if (lhs_numeric && rhs_matrix)
  {
    AddError(node->token, "incompatible types");
    return nullptr;
  }
  else if (lhs_matrix && rhs_numeric)
  {
    const TypePtr &lhs_instantiation_type = lhs->type->template_parameter_types[0];
    if (lhs_instantiation_type != rhs->type)
    {
      AddError(node->token, "incompatible types");
      return nullptr;
    }
    return lhs->type;
  }
  else
  {
    AddError(node->token, "incompatible types");
    return nullptr;
  }
}

bool Analyser::AnnotateExpression(const ExpressionNodePtr &node)
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
    if (symbol->IsVariable() == false)
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
  case Node::Kind::SinglePrecisionNumber:
  {
    SetRV(node, float32_type_);
    break;
  }
  case Node::Kind::DoublePrecisionNumber:
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
  case Node::Kind::AddOp:
  {
    if (AnnotateAddOp(node) == false) return false;
    break;
  }
  case Node::Kind::SubtractOp:
  {
    if (AnnotateSubtractOp(node) == false) return false;
    break;
  }
  case Node::Kind::MultiplyOp:
  {
    if (AnnotateMultiplyOp(node) == false) return false;
    break;
  }
  case Node::Kind::DivideOp:
  {
    if (AnnotateDivideOp(node) == false) return false;
    break;
  }
  case Node::Kind::UnaryMinusOp:
  {
    if (AnnotateUnaryMinusOp(node) == false) return false;
    break;
  }
  case Node::Kind::EqualOp:
  case Node::Kind::NotEqualOp:
  {
    if (AnnotateEqualityOp(node) == false) return false;
    break;
  }
  case Node::Kind::LessThanOp:
  case Node::Kind::LessThanOrEqualOp:
  case Node::Kind::GreaterThanOp:
  case Node::Kind::GreaterThanOrEqualOp:
  {
    if (AnnotateRelationalOp(node) == false) return false;
    break;
  }
  case Node::Kind::AndOp:
  case Node::Kind::OrOp:
  {
    if (AnnotateBinaryLogicalOp(node) == false) return false;
    break;
  }
  case Node::Kind::NotOp:
  {
    if (AnnotateUnaryLogicalOp(node) == false) return false;
    break;
  }
  case Node::Kind::PrefixIncOp:
  case Node::Kind::PrefixDecOp:
  case Node::Kind::PostfixIncOp:
  case Node::Kind::PostfixDecOp:
  {
    if (AnnotateIncDecOp(node) == false) return false;
    break;
  }
  // case Node::Kind::SquareBracketGroup:
  case Node::Kind::IndexOp:
  {
    if (AnnotateIndexOp(node) == false) return false;
    break;
  }
  case Node::Kind::DotOp:
  {
    if (AnnotateDotOp(node) == false) return false;
    break;
  }
  case Node::Kind::InvokeOp:
  {
    if (AnnotateInvokeOp(node) == false) return false;
    break;
  }
  default:
  {
    AddError(node->token, "internal error at '" + node->token.text + "'");
    return false;
  }
  }
  return true;
}

bool Analyser::AnnotateAddOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  TypePtr           type;
  if ((lhs->type->id == TypeId::String) && (rhs->type->id == TypeId::String))
  {
    type = lhs->type;
  }
  else
  {
    type = IsAddSubtractCompatible(node, lhs, rhs);
    if (type == nullptr) return false;
  }
  SetRV(node, type);
  return true;
}

bool Analyser::AnnotateSubtractOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs  = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs  = ConvertToExpressionNodePtr(node->children[1]);
  TypePtr           type = IsAddSubtractCompatible(node, lhs, rhs);
  if (type == nullptr) return false;
  SetRV(node, type);
  return true;
}

bool Analyser::AnnotateMultiplyOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs  = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs  = ConvertToExpressionNodePtr(node->children[1]);
  TypePtr           type = IsMultiplyCompatible(node, lhs, rhs);
  if (type == nullptr) return false;
  SetRV(node, type);
  return true;
}

bool Analyser::AnnotateDivideOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs  = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs  = ConvertToExpressionNodePtr(node->children[1]);
  TypePtr           type = IsDivideCompatible(node, lhs, rhs);
  if (type == nullptr) return false;
  SetRV(node, type);
  return true;
}

bool Analyser::AnnotateUnaryMinusOp(const ExpressionNodePtr &node)
{
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (AnnotateExpression(operand) == false) return false;
  if ((IsNumericType(operand->type->id) == false) && (IsMatrixType(operand->type->id) == false))
  {
    AddError(node->token, "operand must be numeric or matrix type");
    return false;
  }
  SetRV(node, operand->type);
  return true;
}

bool Analyser::AnnotateEqualityOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if ((lhs->type->id == TypeId::Void) || (rhs->type->id == TypeId::Void))
  {
    AddError(node->token, "unable to compare operand(s) of type Void");
    return false;
  }
  const bool lhs_is_concrete_type = lhs->type->id != TypeId::Null;
  const bool rhs_is_concrete_type = rhs->type->id != TypeId::Null;
  if (lhs_is_concrete_type)
  {
    if (rhs_is_concrete_type)
    {
      if (lhs->type != rhs->type)
      {
        AddError(node->token, "incompatible types");
        return false;
      }
    }
    else
    {
      if (lhs->type->IsPrimitiveType())
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
      if (rhs->type->IsPrimitiveType())
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
      // Comparing null to null -- allow
    }
  }
  SetRV(node, bool_type_);
  return true;
}

bool Analyser::AnnotateRelationalOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if ((IsRelationalType(lhs->type->id) == false) || (IsRelationalType(rhs->type->id) == false))
  {
    AddError(node->token, "incompatible types");
    return false;
  }
  if (lhs->type != rhs->type)
  {
    AddError(node->token, "incompatible types");
    return false;
  }
  SetRV(node, bool_type_);
  return true;
}

bool Analyser::AnnotateBinaryLogicalOp(const ExpressionNodePtr &node)
{
  for (const NodePtr &child : node->children)
  {
    if (AnnotateExpression(ConvertToExpressionNodePtr(child)) == false) return false;
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if ((lhs->type->id != TypeId::Bool) || (rhs->type->id != TypeId::Bool))
  {
    AddError(node->token, "boolean operands expected");
    return false;
  }
  SetRV(node, bool_type_);
  return true;
}

bool Analyser::AnnotateUnaryLogicalOp(const ExpressionNodePtr &node)
{
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (AnnotateExpression(operand) == false) return false;
  if (operand->type->id != TypeId::Bool)
  {
    AddError(node->token, "boolean operand expected");
    return false;
  }
  SetRV(node, bool_type_);
  return true;
}

bool Analyser::AnnotateIncDecOp(const ExpressionNodePtr &node)
{
  ExpressionNodePtr operand = ConvertToExpressionNodePtr(node->children[0]);
  if (AnnotateExpression(operand) == false) return false;
  if (IsWriteable(operand) == false) return false;
  if (IsIntegralType(operand->type->id) == false)
  {
    AddError(node->token, "integral type expected");
    return false;
  }
  SetRV(node, operand->type);
  return true;
}

bool Analyser::AnnotateIndexOp(const ExpressionNodePtr &node)
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
    if (symbol->IsVariable() == false)
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
    if (AnnotateExpression(lhs) == false) return false;
  }

  if ((lhs->category == ExpressionNode::Category::Type) ||
      (lhs->category == ExpressionNode::Category::Function))
  {
    AddError(lhs->token, "operand does not support index operator");
    return false;
  }

  int num_expected_rhs_operands;
  if (IsMatrixType(lhs->type->id))
  {
    // row, column
    num_expected_rhs_operands = 2;
  }
  else if (IsArrayType(lhs->type->id))
  {
    // index
    num_expected_rhs_operands = 1;
  }
  else
  {
    // Only matrices and arrays support indexing
    AddError(lhs->token, "operand does not support index operator");
    return false;
  }

  const int num_supplied_rhs_operands = (int)node->children.size() - 1;
  if (num_supplied_rhs_operands != num_expected_rhs_operands)
  {
    AddError(lhs->token, "incorrect number of operands for index operator");
    return false;
  }

  for (int i = 1; i <= num_supplied_rhs_operands; ++i)
  {
    const NodePtr &   operand      = node->children[std::size_t(i)];
    ExpressionNodePtr operand_node = ConvertToExpressionNodePtr(operand);
    if (AnnotateExpression(operand_node) == false) return false;
    if (IsIntegralType(operand_node->type->id) == false)
    {
      AddError(operand_node->token, "integral subscript expected");
      return false;
    }
  }

  SetLV(node, lhs->type->template_parameter_types[0]);
  return true;
}

bool Analyser::AnnotateDotOp(const ExpressionNodePtr &node)
{
  ExpressionNodePtr  lhs         = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr  rhs         = ConvertToExpressionNodePtr(node->children[1]);
  const std::string &member_name = rhs->token.text;
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
    if (AnnotateExpression(lhs) == false) return false;
  }

  if (lhs->category == ExpressionNode::Category::Function)
  {
    AddError(lhs->token, "operand does not support member-access operator");
    return false;
  }

  const bool lhs_is_instance = (lhs->category == ExpressionNode::Category::Variable) ||
                               (lhs->category == ExpressionNode::Category::LV) ||
                               (lhs->category == ExpressionNode::Category::RV);

  if (lhs->type->IsPrimitiveType())
  {
    AddError(lhs->token,
             "primitive type '" + lhs->type->name + "' does not support member-access operator");
    return false;
  }
  SymbolPtr symbol;
  if (lhs->type->category == Type::Category::TemplateInstantiation)
    symbol = lhs->type->template_type->symbols->Find(member_name);
  else
    symbol = lhs->type->symbols->Find(member_name);
  if (symbol == nullptr)
  {
    AddError(lhs->token,
             "type '" + lhs->type->name + "' has no member named '" + member_name + "'");
    return false;
  }
  if (symbol->IsFunctionGroup())
  {
    FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
    // Instance function	(lhs_is_instance true)
    // Type function		(lhs_is_instance false)
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

bool Analyser::AnnotateInvokeOp(const ExpressionNodePtr &node)
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
    if (AnnotateExpression(lhs) == false) return false;
  }

  std::vector<TypePtr> supplied_parameter_types;
  for (int i = 1; i < (int)node->children.size(); ++i)
  {
    const NodePtr &   supplied_parameter      = node->children[std::size_t(i)];
    ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
    if (AnnotateExpression(supplied_parameter_node) == false) return false;
    supplied_parameter_types.push_back(supplied_parameter_node->type);
  }

  if (lhs->category == ExpressionNode::Category::Function)
  {
    // user function (lhs->type is nullptr)
    // opcode function (lhs->type is nullptr)
    // opcode type function
    // opcode instance function
    std::vector<TypePtr> output_parameter_types;
    FunctionPtr          f =
        FindMatchingFunction(lhs->fg, lhs->type, supplied_parameter_types, output_parameter_types);
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
      if (lhs->function_invoked_on_instance == false)
      {
        AddError(lhs->token,
                 "function '" + lhs->fg->name + "' is an instance function, not a type function");
        return false;
      }
    }

    for (int i = 1; i < (int)node->children.size(); ++i)
    {
      const NodePtr &   supplied_parameter      = node->children[std::size_t(i)];
      ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
      supplied_parameter_node->type             = output_parameter_types[std::size_t(i - 1)];
    }

    TypePtr actual_return_type = ConvertType(f->return_type, lhs->type);
    SetRV(node, actual_return_type);
    node->function = f;
    return true;
  }
  else if (lhs->category == ExpressionNode::Category::Type)
  {
    // Constructor
    if (lhs->type->IsPrimitiveType())
    {
      AddError(lhs->token, "primitive type '" + lhs->type->name + "' is not constructible");
      return false;
    }
    SymbolPtr symbol;
    if (lhs->type->category == Type::Category::TemplateInstantiation)
      symbol = lhs->type->template_type->symbols->Find(CONSTRUCTOR);
    else
      symbol = lhs->type->symbols->Find(CONSTRUCTOR);
    if (symbol == nullptr)
    {
      AddError(lhs->token,
               "unable to find matching constructor for type '" + lhs->type->name + "'");
      return false;
    }
    FunctionGroupPtr     fg = ConvertToFunctionGroupPtr(symbol);
    std::vector<TypePtr> output_parameter_types;
    FunctionPtr          f =
        FindMatchingFunction(fg, lhs->type, supplied_parameter_types, output_parameter_types);
    if (f == nullptr)
    {
      // No matching constructor, or ambiguous
      AddError(lhs->token,
               "unable to find matching constructor for type '" + lhs->type->name + "'");
      return false;
    }

    for (int i = 1; i < (int)node->children.size(); ++i)
    {
      const NodePtr &   supplied_parameter      = node->children[std::size_t(i)];
      ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
      supplied_parameter_node->type             = output_parameter_types[std::size_t(i - 1)];
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

TypePtr Analyser::ConvertType(TypePtr type, TypePtr instantiated_template_type)
{
  TypePtr converted_type;
  if (type->id == TypeId::TemplateInstantiation)
  {
    converted_type = instantiated_template_type;
  }
  else if (type->id == TypeId::TemplateParameter1)
  {
    converted_type = instantiated_template_type->template_parameter_types[0];
  }
  else if (type->id == TypeId::TemplateParameter2)
  {
    converted_type = instantiated_template_type->template_parameter_types[1];
  }
  else
    converted_type = type;
  return converted_type;
}

FunctionPtr Analyser::FindMatchingFunction(const FunctionGroupPtr &fg, const TypePtr &type,
                                           const std::vector<TypePtr> &input_types,
                                           std::vector<TypePtr> &      output_types)
{
  std::vector<FunctionPtr>          function_array;
  std::vector<std::vector<TypePtr>> output_types_array;
  for (const FunctionPtr &function : fg->functions)
  {
    const int num_types = (int)function->parameter_types.size();
    if ((int)input_types.size() != num_types) continue;
    if (num_types == 0)
    {
      output_types.clear();
      return function;
    }
    bool                 match = true;
    std::vector<TypePtr> temp_output_types;
    for (int i = 0; i < num_types; ++i)
    {
      const TypePtr &input_type = input_types[std::size_t(i)];
      if (input_type->id == TypeId::Void)
      {
        match = false;
        break;
      }
      TypePtr expected_type = ConvertType(function->parameter_types[std::size_t(i)], type);
      if (expected_type->id == TypeId::Numeric)
      {
        if (IsNumericType(input_type->id) == false)
        {
          match = false;
          break;
        }
        temp_output_types.push_back(input_type);
      }
      else if (input_type->id == TypeId::Null)
      {
        if (expected_type->IsPrimitiveType())
        {
          match = false;
          break;
        }
        temp_output_types.push_back(expected_type);
      }
      else
      {
        if (input_type != expected_type)
        {
          match = false;
          break;
        }
        temp_output_types.push_back(input_type);
      }
    }
    if (match)
    {
      output_types_array.push_back(temp_output_types);
      function_array.push_back(function);
    }
  }
  if (function_array.size() == 1)
  {
    output_types = output_types_array[0];
    return function_array[0];
  }
  // Matching function not found, or ambiguous
  return nullptr;
}

TypePtr Analyser::FindType(const ExpressionNodePtr &node)
{
  SymbolPtr symbol = FindSymbol(node);
  if (symbol == nullptr) return nullptr;
  if (symbol->IsType() == false) return nullptr;
  return ConvertToTypePtr(symbol);
}

SymbolPtr Analyser::FindSymbol(const ExpressionNodePtr &node)
{
  if (node->kind == Node::Kind::Template)
  {
    ExpressionNodePtr  identifier_node = ConvertToExpressionNodePtr(node->children[0]);
    const std::string &name            = identifier_node->token.text;
    if (name == "Matrix")
    {
      if (node->children.size() != 2)
      {
        // requires 1 template parameter
        return nullptr;
      }
      ExpressionNodePtr subtype_node = ConvertToExpressionNodePtr(node->children[1]);
      if (subtype_node->kind != Node::Kind::Identifier) return nullptr;
      const std::string &subtype_name = subtype_node->token.text;
      if (subtype_name == "Float64")
        return matrix_float64_type_;
      else if (subtype_name == "Float32")
        return matrix_float32_type_;
      return nullptr;
    }
    else if (name == "Array")
    {
      if (node->children.size() != 2)
      {
        // requires 1 template parameter
        return nullptr;
      }
      ExpressionNodePtr subtype_node = ConvertToExpressionNodePtr(node->children[1]);
      TypePtr           subtype      = FindType(subtype_node);
      if (subtype == nullptr) return nullptr;
      std::string template_name = name + "<" + subtype->name + ">";
      SymbolPtr   symbol        = SearchSymbolTables(template_name);
      if (symbol)
        return ConvertToTypePtr(symbol);
      else
      {
        // Not seen this template instantation before, so instantiate it now
        TypePtr type = CreateTemplateInstantiationType(template_name, TypeId::Array,
                                                       array_template_type_, {subtype});
        return type;
      }
    }
    else
    {
      return nullptr;
    }
  }
  else  // (node->kind == Node::Kind::Identifier)
  {
    const std::string &name = node->token.text;
    if (name != "Matrix")
    {
      SymbolPtr symbol = SearchSymbolTables(name);
      if (symbol) return symbol;
      return nullptr;
    }
    else
    {
      return matrix_float64_type_;
    }
  }
}

SymbolPtr Analyser::SearchSymbolTables(const std::string &name)
{
  SymbolPtr symbol = symbols_->Find(name);
  if (symbol) return symbol;
  auto it  = blocks_.rbegin();
  auto end = blocks_.rend();
  while (it != end)
  {
    const BlockNodePtr &block_node = *it;
    symbol                         = block_node->symbols->Find(name);
    if (symbol) return symbol;
    ++it;
  }
  return nullptr;
}

void Analyser::SetVariable(const ExpressionNodePtr &node, const VariablePtr &variable)
{
  node->category = ExpressionNode::Category::Variable;
  node->variable = variable;
  node->type     = variable->type;
  node->fg       = nullptr;
  node->function = nullptr;
}

void Analyser::SetLV(const ExpressionNodePtr &node, const TypePtr &type)
{
  node->category = ExpressionNode::Category::LV;
  node->variable = nullptr;
  node->type     = type;
  node->fg       = nullptr;
  node->function = nullptr;
}

void Analyser::SetRV(const ExpressionNodePtr &node, const TypePtr &type)
{
  node->category = ExpressionNode::Category::RV;
  node->variable = nullptr;
  node->type     = type;
  node->fg       = nullptr;
  node->function = nullptr;
}

void Analyser::SetType(const ExpressionNodePtr &node, const TypePtr &type)
{
  node->category = ExpressionNode::Category::Type;
  node->variable = nullptr;
  node->type     = type;
  node->fg       = nullptr;
  node->function = nullptr;
}

void Analyser::SetFunction(const ExpressionNodePtr &node, const FunctionGroupPtr &fg,
                           const TypePtr &fg_owner, const bool function_invoked_on_instance)
{
  node->category                     = ExpressionNode::Category::Function;
  node->variable                     = nullptr;
  node->type                         = fg_owner;
  node->fg                           = fg;
  node->function_invoked_on_instance = function_invoked_on_instance;
  node->function                     = nullptr;
}

TypePtr Analyser::CreatePrimitiveType(const std::string &name, const TypeId id)
{
  TypePtr type = CreateType(name, Type::Category::Primitive, id);
  symbols_->Add(name, type);
  return type;
}

TypePtr Analyser::CreateTemplateType(const std::string &name, const TypeId id)
{
  TypePtr type  = CreateType(name, Type::Category::Template, id);
  type->symbols = CreateSymbolTable();
  symbols_->Add(name, type);
  return type;
}

TypePtr Analyser::CreateTemplateInstantiationType(
    const std::string &name, const TypeId id, const TypePtr &template_type,
    const std::vector<TypePtr> &template_parameter_types)
{
  TypePtr type                   = CreateType(name, Type::Category::TemplateInstantiation, id);
  type->template_type            = template_type;
  type->template_parameter_types = template_parameter_types;
  symbols_->Add(name, type);
  return type;
}

TypePtr Analyser::CreateClassType(const std::string &name, const TypeId id)
{
  TypePtr type  = CreateType(name, Type::Category::Class, id);
  type->symbols = CreateSymbolTable();
  symbols_->Add(name, type);
  return type;
}

FunctionPtr Analyser::CreateUserFunction(const std::string &             name,
                                         const std::vector<TypePtr> &    parameter_types,
                                         const std::vector<VariablePtr> &parameter_variables,
                                         const TypePtr &                 return_type)
{
  FunctionPtr function          = CreateFunction(Function::Kind::UserFunction, name);
  function->parameter_types     = parameter_types;
  function->parameter_variables = parameter_variables;
  function->return_type         = return_type;
  function->opcode              = Opcode::Unknown;
  function->index               = 0;
  return function;
}

FunctionPtr Analyser::CreateOpcodeFunction(const std::string &name, const Function::Kind kind,
                                           const std::vector<TypePtr> &parameter_types,
                                           const TypePtr &return_type, const Opcode opcode)
{
  FunctionPtr function      = CreateFunction(kind, name);
  function->parameter_types = parameter_types;
  function->return_type     = return_type;
  function->opcode          = opcode;
  function->index           = 0;
  return function;
}

void Analyser::AddFunction(const SymbolTablePtr &symbols, const FunctionPtr &function)
{
  FunctionGroupPtr fg;
  SymbolPtr        symbol = symbols->Find(function->name);
  if (symbol)
  {
    fg = ConvertToFunctionGroupPtr(symbol);
  }
  else
  {
    fg = CreateFunctionGroup(function->name);
    symbols->Add(function->name, fg);
  }
  fg->functions.push_back(function);
}

void Analyser::CreateOpcodeFunction(const std::string &         name,
                                    const std::vector<TypePtr> &parameter_types,
                                    const TypePtr &return_type, const Opcode opcode)
{
  AddFunction(symbols_, CreateOpcodeFunction(name, Function::Kind::OpcodeFunction, parameter_types,
                                             return_type, opcode));
}

void Analyser::CreateOpcodeTypeFunction(const TypePtr &type, const std::string &name,
                                        const std::vector<TypePtr> &parameter_types,
                                        const TypePtr &return_type, const Opcode opcode)
{
  AddFunction(type->symbols, CreateOpcodeFunction(name, Function::Kind::OpcodeTypeFunction,
                                                  parameter_types, return_type, opcode));
}

void Analyser::CreateOpcodeInstanceFunction(const TypePtr &type, const std::string &name,
                                            const std::vector<TypePtr> &parameter_types,
                                            const TypePtr &return_type, const Opcode opcode)
{
  AddFunction(type->symbols, CreateOpcodeFunction(name, Function::Kind::OpcodeInstanceFunction,
                                                  parameter_types, return_type, opcode));
}

}  // namespace vm
}  // namespace fetch
