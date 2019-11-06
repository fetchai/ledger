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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/analyser.hpp"
#include "vm/array.hpp"
#include "vm/map.hpp"
#include "vm/matrix.hpp"
#include "vm/sharded_state.hpp"
#include "vm/state.hpp"
#include "vm/string.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>
#include <typeinfo>

namespace fetch {
namespace vm {

std::string Analyser::CONSTRUCTOR       = "[Constructor]";
std::string Analyser::GET_INDEXED_VALUE = "[GetIndexedValue]";
std::string Analyser::SET_INDEXED_VALUE = "[SetIndexedValue]";

void Analyser::Initialise()
{
  operator_map_ = {{NodeKind::Equal, Operator::Equal},
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
  type_set_            = StringSet();
  type_info_array_     = TypeInfoArray(TypeIds::NumReserved);
  type_info_map_       = TypeInfoMap();
  registered_types_    = RegisteredTypes();
  function_info_array_ = FunctionInfoArray();
  function_map_        = FunctionMap();
  symbols_             = CreateSymbolTable();

  CreatePrimitiveType("Null", TypeIndex(typeid(std::nullptr_t)), false, TypeIds::Null, null_type_);
  CreatePrimitiveType("InitialiserList", TypeIndex(typeid(InitialiserListPlaceholder)), false,
                      TypeIds::InitialiserList, initialiser_list_type_);
  CreatePrimitiveType("Void", TypeIndex(typeid(void)), false, TypeIds::Void, void_type_);
  CreatePrimitiveType("Bool", TypeIndex(typeid(bool)), true, TypeIds::Bool, bool_type_);
  CreatePrimitiveType("Int8", TypeIndex(typeid(int8_t)), true, TypeIds::Int8, int8_type_);
  CreatePrimitiveType("UInt8", TypeIndex(typeid(uint8_t)), true, TypeIds::UInt8, uint8_type_);
  CreatePrimitiveType("Int16", TypeIndex(typeid(int16_t)), true, TypeIds::Int16, int16_type_);
  CreatePrimitiveType("UInt16", TypeIndex(typeid(uint16_t)), true, TypeIds::UInt16, uint16_type_);
  CreatePrimitiveType("Int32", TypeIndex(typeid(int32_t)), true, TypeIds::Int32, int32_type_);
  CreatePrimitiveType("UInt32", TypeIndex(typeid(uint32_t)), true, TypeIds::UInt32, uint32_type_);
  CreatePrimitiveType("Int64", TypeIndex(typeid(int64_t)), true, TypeIds::Int64, int64_type_);
  CreatePrimitiveType("UInt64", TypeIndex(typeid(uint64_t)), true, TypeIds::UInt64, uint64_type_);
  CreatePrimitiveType("Float32", TypeIndex(typeid(float)), true, TypeIds::Float32, float32_type_);
  CreatePrimitiveType("Float64", TypeIndex(typeid(double)), true, TypeIds::Float64, float64_type_);
  CreatePrimitiveType("Fixed32", TypeIndex(typeid(fixed_point::fp32_t)), true, TypeIds::Fixed32,
                      fixed32_type_);
  CreatePrimitiveType("Fixed64", TypeIndex(typeid(fixed_point::fp64_t)), true, TypeIds::Fixed64,
                      fixed64_type_);

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

  TypePtrArray const integer_types = {int8_type_,  uint8_type_,  int16_type_, uint16_type_,
                                      int32_type_, uint32_type_, int64_type_, uint64_type_};
  TypePtrArray const number_types  = {int8_type_,    uint8_type_,   int16_type_,   uint16_type_,
                                     int32_type_,   uint32_type_,  int64_type_,   uint64_type_,
                                     float32_type_, float64_type_, fixed32_type_, fixed64_type_};
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

  CreateTemplateType("Array", TypeIndex(typeid(IArray)), {any_type_}, TypeIds::Unknown,
                     array_type_);
  CreateTemplateType("Map", TypeIndex(typeid(IMap)), {any_type_, any_type_}, TypeIds::Unknown,
                     map_type_);
  CreateTemplateType("State", TypeIndex(typeid(IState)), {any_type_}, TypeIds::Unknown,
                     state_type_);
  CreateTemplateType("ShardedState", TypeIndex(typeid(IShardedState)), {any_type_},
                     TypeIds::Unknown, sharded_state_type_);
}

void Analyser::UnInitialise()
{
  operator_map_ = OperatorMap();
  type_map_.Reset();
  type_map_            = TypeMap();
  type_set_            = StringSet();
  type_info_array_     = TypeInfoArray();
  type_info_map_       = TypeInfoMap();
  registered_types_    = RegisteredTypes();
  function_info_array_ = FunctionInfoArray();
  function_map_        = FunctionMap();
  if (symbols_)
  {
    symbols_->Reset();
    symbols_ = nullptr;
  }
  null_type_                = nullptr;
  void_type_                = nullptr;
  bool_type_                = nullptr;
  int8_type_                = nullptr;
  uint8_type_               = nullptr;
  int16_type_               = nullptr;
  uint16_type_              = nullptr;
  int32_type_               = nullptr;
  uint32_type_              = nullptr;
  int64_type_               = nullptr;
  uint64_type_              = nullptr;
  float32_type_             = nullptr;
  float64_type_             = nullptr;
  fixed32_type_             = nullptr;
  fixed64_type_             = nullptr;
  string_type_              = nullptr;
  address_type_             = nullptr;
  template_parameter1_type_ = nullptr;
  template_parameter2_type_ = nullptr;
  any_type_                 = nullptr;
  any_primitive_type_       = nullptr;
  any_integer_type_         = nullptr;
  any_floating_point_type_  = nullptr;
  array_type_               = nullptr;
  map_type_                 = nullptr;
  state_type_               = nullptr;
  address_type_             = nullptr;
  sharded_state_type_       = nullptr;
  initialiser_list_type_    = nullptr;
}

void Analyser::CreateClassType(std::string const &name, TypeIndex type_index)
{
  TypePtr type;
  CreateClassType(name, type_index, TypeIds::Unknown, type);
}

void Analyser::CreateTemplateType(std::string const &name, TypeIndex type_index,
                                  TypeIndexArray const &allowed_types_index_array)
{
  TypePtr type;
  CreateTemplateType(name, type_index, GetTypes(allowed_types_index_array), TypeIds::Unknown, type);
}

void Analyser::CreateTemplateInstantiationType(
    TypeIndex type_index, TypeIndex template_type_index,
    TypeIndexArray const &template_parameter_type_index_array)
{
  TypePtr type;
  CreateTemplateInstantiationType(type_index, GetType(template_type_index),
                                  GetTypes(template_parameter_type_index_array), TypeIds::Unknown,
                                  type);
}

void Analyser::CreateFreeFunction(std::string const &   name,
                                  TypeIndexArray const &parameter_type_index_array,
                                  TypeIndex return_type_index, Handler const &handler,
                                  ChargeAmount static_charge)
{
  CreateFreeFunction(name, GetTypes(parameter_type_index_array), GetType(return_type_index),
                     handler, static_charge);
}

void Analyser::CreateConstructor(TypeIndex             type_index,
                                 TypeIndexArray const &parameter_type_index_array,
                                 Handler const &handler, ChargeAmount static_charge)
{
  CreateConstructor(GetType(type_index), GetTypes(parameter_type_index_array), handler,
                    static_charge);
}

void Analyser::CreateStaticMemberFunction(TypeIndex type_index, std::string const &function_name,
                                          TypeIndexArray const &parameter_type_index_array,
                                          TypeIndex return_type_index, Handler const &handler,
                                          ChargeAmount static_charge)
{
  CreateStaticMemberFunction(GetType(type_index), function_name,
                             GetTypes(parameter_type_index_array), GetType(return_type_index),
                             handler, static_charge);
}

void Analyser::CreateMemberFunction(TypeIndex type_index, std::string const &function_name,
                                    TypeIndexArray const &parameter_type_index_array,
                                    TypeIndex return_type_index, Handler const &handler,
                                    ChargeAmount static_charge)
{
  CreateMemberFunction(GetType(type_index), function_name, GetTypes(parameter_type_index_array),
                       GetType(return_type_index), handler, static_charge);
}

void Analyser::EnableOperator(TypeIndex type_index, Operator op)
{
  EnableOperator(GetType(type_index), op);
}

void Analyser::EnableLeftOperator(TypeIndex type_index, Operator op)
{
  EnableLeftOperator(GetType(type_index), op);
}

void Analyser::EnableRightOperator(TypeIndex type_index, Operator op)
{
  EnableRightOperator(GetType(type_index), op);
}

void Analyser::EnableIndexOperator(TypeIndex             type_index,
                                   TypeIndexArray const &input_type_index_array,
                                   TypeIndex output_type_index, Handler const &get_handler,
                                   Handler const &set_handler, ChargeAmount get_static_charge,
                                   ChargeAmount set_static_charge)
{
  EnableIndexOperator(GetType(type_index), GetTypes(input_type_index_array),
                      GetType(output_type_index), get_handler, set_handler, get_static_charge,
                      set_static_charge);
}

bool Analyser::Analyse(BlockNodePtr const &root, std::vector<std::string> &errors)
{
  root_ = root;
  blocks_.clear();
  loops_.clear();
  state_constructor_ =
      function_map_.Find(BuildUniqueName(state_type_, CONSTRUCTOR, {string_type_}, state_type_));
  sharded_state_constructor_ = function_map_.Find(
      BuildUniqueName(sharded_state_type_, CONSTRUCTOR, {string_type_}, sharded_state_type_));
  assert(state_constructor_ && sharded_state_constructor_);
  state_definitions_.Clear();
  contract_definitions_.Clear();
  function_     = nullptr;
  use_any_node_ = nullptr;
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
    state_constructor_         = nullptr;
    sharded_state_constructor_ = nullptr;
    state_definitions_.Clear();
    contract_definitions_.Clear();
    function_     = nullptr;
    use_any_node_ = nullptr;
    errors_.clear();
    return false;
  }

  AnnotateBlock(root_);

  root_ = nullptr;
  blocks_.clear();
  loops_.clear();
  state_constructor_         = nullptr;
  sharded_state_constructor_ = nullptr;
  state_definitions_.Clear();
  contract_definitions_.Clear();
  function_     = nullptr;
  use_any_node_ = nullptr;

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
  std::ostringstream stream;
  stream << filename_ << ": line " << line << ": error: " << message;
  errors_.push_back(stream.str());
}

void Analyser::BuildBlock(BlockNodePtr const &block_node)
{
  blocks_.push_back(block_node);
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      BuildFile(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::PersistentStatement:
    {
      BuildPersistentStatement(child);
      break;
    }
    case NodeKind::ContractDefinition:
    {
      BuildContractDefinition(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::FunctionDefinition:
    {
      BuildFunctionDefinition(ConvertToBlockNodePtr(child));
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
  blocks_.pop_back();
}

void Analyser::BuildFile(BlockNodePtr const &file_node)
{
  filename_          = file_node->text;
  file_node->symbols = CreateSymbolTable();
  BuildBlock(file_node);
}

void Analyser::BuildPersistentStatement(NodePtr const &persistent_statement_node)
{
  ExpressionNodePtr state_name_node =
      ConvertToExpressionNodePtr(persistent_statement_node->children[0]);
  ExpressionNodePtr modifier_node =
      ConvertToExpressionNodePtr(persistent_statement_node->children[1]);
  ExpressionNodePtr  type_node = ConvertToExpressionNodePtr(persistent_statement_node->children[2]);
  std::string const &state_name = state_name_node->text;
  if (state_name == "any")
  {
    AddError(state_name_node->line, "invalid state name");
    return;
  }
  if (state_definitions_.Find(state_name))
  {
    AddError(state_name_node->line, "state '" + state_name + "' is already defined");
    return;
  }
  TypePtr managed_type = FindType(type_node);
  if (!managed_type)
  {
    AddError(type_node->line, "unknown type '" + type_node->text + "'");
    return;
  }
  TypePtr template_type;
  if (modifier_node && (modifier_node->text == "sharded"))
  {
    template_type = sharded_state_type_;
  }
  else
  {
    template_type = state_type_;
  }
  std::string instantation_name = template_type->name + "<" + managed_type->name + ">";
  TypePtr     instantation_type;
  auto        symbol = SearchSymbols(instantation_name);
  if (symbol)
  {
    // The instantiation already exists
    instantation_type = ConvertToTypePtr(symbol);
  }
  else
  {
    // The instantiation doesn't already exist, so create it now
    instantation_type = InternalCreateTemplateInstantiationType(
        TypeKind::UserDefinedTemplateInstantiation, template_type, {managed_type});
    root_->symbols->Add(instantation_type);
  }
  state_name_node->type = instantation_type;
  type_node->type       = managed_type;
  state_definitions_.Add(state_name, instantation_type);
}

void Analyser::BuildContractDefinition(BlockNodePtr const &contract_definition_node)
{
  contract_definition_node->symbols = CreateSymbolTable();
  ExpressionNodePtr contract_name_node =
      ConvertToExpressionNodePtr(contract_definition_node->children[0]);
  std::string const &contract_name = contract_name_node->text;
  // check symbols_ as well here?
  if (contract_definitions_.Find(contract_name) || root_->symbols->Find(contract_name))
  {
    AddError(contract_name_node->line, "symbol '" + contract_name + "' is already defined");
    return;
  }
  TypePtr contract_type  = CreateType(TypeKind::UserDefinedContract, contract_name);
  contract_type->symbols = CreateSymbolTable();
  blocks_.push_back(contract_definition_node);
  for (NodePtr const &contract_function_prototype_node : contract_definition_node->block_children)
  {
    ExpressionNodePtr      function_name_node;
    ExpressionNodePtrArray parameter_nodes;
    TypePtrArray           parameter_types;
    VariablePtrArray       parameter_variables;
    TypePtr                return_type;
    if (!BuildFunctionPrototype(contract_function_prototype_node, function_name_node,
                                parameter_nodes, parameter_types, parameter_variables, return_type))
    {
      continue;
    }
    std::string const &function_name = function_name_node->text;
    FunctionGroupPtr   function_group;
    SymbolPtr          symbol = contract_definition_node->symbols->Find(function_name);
    if (symbol)
    {
      if (!symbol->IsFunctionGroup())
      {
        AddError(function_name_node->line, "symbol '" + function_name + "' is already defined");
        continue;
      }
      function_group = ConvertToFunctionGroupPtr(symbol);
      TypePtrArray dummy;
      if (FindFunction(nullptr, function_group, parameter_nodes, dummy))
      {
        AddError(
            function_name_node->line,
            "function '" + function_name + "' is already defined with the same parameter types");
        continue;
      }
    }
    else
    {
      function_group = CreateFunctionGroup(function_name);
      contract_definition_node->symbols->Add(function_group);
    }
    FunctionPtr function = CreateUserDefinedContractFunction(
        contract_type, function_name, parameter_types, parameter_variables, return_type);
    function_group->functions.push_back(function);
    function_name_node->function = function;
  }
  blocks_.pop_back();
  for (auto const &it : contract_definition_node->symbols->map)
  {
    contract_type->symbols->Add(it.second);
  }
  contract_name_node->type = contract_type;
  contract_definitions_.Add(contract_name, contract_type);
}

void Analyser::BuildFunctionDefinition(BlockNodePtr const &function_definition_node)
{
  function_definition_node->symbols = CreateSymbolTable();

  ExpressionNodePtr      function_name_node;
  ExpressionNodePtrArray parameter_nodes;
  TypePtrArray           parameter_types;
  VariablePtrArray       parameter_variables;
  TypePtr                return_type;
  if (!BuildFunctionPrototype(function_definition_node, function_name_node, parameter_nodes,
                              parameter_types, parameter_variables, return_type))
  {
    return;
  }
  for (auto const &parameter_variable : parameter_variables)
  {
    function_definition_node->symbols->Add(parameter_variable);
  }
  std::string const &function_name = function_name_node->text;
  if (contract_definitions_.Find(function_name))
  {
    AddError(function_name_node->line, "symbol '" + function_name + "' is already defined");
    return;
  }
  // check symbols_ as well here?
  FunctionGroupPtr function_group;
  SymbolPtr        symbol = root_->symbols->Find(function_name);
  if (symbol)
  {
    if (!symbol->IsFunctionGroup())
    {
      AddError(function_name_node->line, "symbol '" + function_name + "' is already defined");
      return;
    }
    function_group = ConvertToFunctionGroupPtr(symbol);
    TypePtrArray dummy;
    if (FindFunction(nullptr, function_group, parameter_nodes, dummy))
    {
      AddError(function_name_node->line,
               "function '" + function_name + "' is already defined with the same parameter types");
      return;
    }
  }
  else
  {
    function_group = CreateFunctionGroup(function_name);
    root_->symbols->Add(function_group);
  }
  FunctionPtr function = CreateUserDefinedFreeFunction(function_name, parameter_types,
                                                       parameter_variables, return_type);
  function_group->functions.push_back(function);
  function_name_node->function = function;
  BuildBlock(function_definition_node);
}

bool Analyser::BuildFunctionPrototype(NodePtr const &         prototype_node,
                                      ExpressionNodePtr &     function_name_node,
                                      ExpressionNodePtrArray &parameter_nodes,
                                      TypePtrArray &          parameter_types,
                                      VariablePtrArray &parameter_variables, TypePtr &return_type)
{
  function_name_node = ConvertToExpressionNodePtr(prototype_node->children[1]);

  parameter_nodes.clear();
  parameter_types.clear();
  parameter_variables.clear();
  StringSet parameter_names;

  auto const count          = prototype_node->children.size();
  auto const num_parameters = (count - 3) / 2;
  int        problems       = 0;

  for (std::size_t i = 0; i < num_parameters; ++i)
  {
    ExpressionNodePtr parameter_node =
        ConvertToExpressionNodePtr(prototype_node->children[2 + i * 2]);
    std::string const &parameter_name = parameter_node->text;
    if (parameter_names.Find(parameter_name))
    {
      AddError(parameter_node->line, "parameter name '" + parameter_name + "' is already defined");
      ++problems;
      continue;
    }
    parameter_names.Add(parameter_name);
    ExpressionNodePtr parameter_type_node =
        ConvertToExpressionNodePtr(prototype_node->children[3 + i * 2]);
    TypePtr parameter_type = FindType(parameter_type_node);
    if (!parameter_type)
    {
      AddError(parameter_type_node->line, "unknown type '" + parameter_type_node->text + "'");
      ++problems;
      continue;
    }
    VariablePtr parameter_variable =
        CreateVariable(VariableKind::Parameter, parameter_name, parameter_type);
    parameter_node->variable = parameter_variable;
    parameter_node->type     = parameter_variable->type;
    parameter_types.push_back(parameter_type);
    parameter_variables.push_back(parameter_variable);
    parameter_nodes.push_back(parameter_node);
  }
  ExpressionNodePtr return_type_node =
      ConvertToExpressionNodePtr(prototype_node->children[std::size_t(count - 1)]);
  if (return_type_node)
  {
    return_type = FindType(return_type_node);
    if (!return_type)
    {
      AddError(return_type_node->line, "unknown type '" + return_type_node->text + "'");
      ++problems;
    }
  }
  else
  {
    return_type = void_type_;
  }
  return (problems == 0);
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
  bool const is_loop = block_node->node_kind == NodeKind::WhileStatement ||
                       block_node->node_kind == NodeKind::ForStatement;
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
    case NodeKind::PersistentStatement:
    case NodeKind::ContractDefinition:
    {
      // nothing to do
      break;
    }
    case NodeKind::FunctionDefinition:
    {
      AnnotateFunctionDefinition(ConvertToBlockNodePtr(child));
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
    case NodeKind::UseStatement:
    {
      AnnotateUseStatement(block_node, child);
      break;
    }
    case NodeKind::UseAnyStatement:
    {
      AnnotateUseAnyStatement(block_node, child);
      break;
    }
    case NodeKind::ContractStatement:
    {
      AnnotateContractStatement(block_node, child);
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
      if (loops_.empty())
      {
        AddError(child->line, "break statement is not inside a while or for loop");
      }
      break;
    }
    case NodeKind::ContinueStatement:
    {
      if (loops_.empty())
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
    case NodeKind::InitialiserList:
    {
      AddError(child->line, "cannot infer list type");
      break;
    }
    default:
    {
      AnnotateExpression(ConvertToExpressionNodePtr(child));
      break;
    }
    }  // switch
  }

  for (auto const &it : block_node->symbols->map)
  {
    SymbolPtr const &symbol = it.second;
    if (!symbol->IsVariable())
    {
      continue;
    }
    VariablePtr variable = ConvertToVariablePtr(symbol);
    if (variable->variable_kind == VariableKind::Use)
    {
      if (!variable->referenced)
      {
        AddError(block_node->block_terminator_line,
                 "state variable '" + variable->name + "' is not referenced in block");
      }
    }
    else if (variable->variable_kind == VariableKind::UseAny)
    {
      if (variable->referenced)
      {
        ExpressionNodePtr child =
            CreateExpressionNode(NodeKind::Identifier, variable->name, use_any_node_->line);
        child->variable         = variable;
        FunctionPtr constructor = (variable->type->template_type == sharded_state_type_)
                                      ? sharded_state_constructor_
                                      : state_constructor_;
        child->function = constructor;
        use_any_node_->children.push_back(child);
      }
    }
  }

  if (is_loop)
  {
    loops_.pop_back();
  }
  blocks_.pop_back();
}

void Analyser::AnnotateFile(BlockNodePtr const &file_node)
{
  filename_ = file_node->text;
  AnnotateBlock(file_node);
}

void Analyser::AnnotateFunctionDefinition(BlockNodePtr const &function_definition_node)
{
  ExpressionNodePtr function_name_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  function_     = function_name_node->function;
  use_any_node_ = nullptr;
  AnnotateBlock(function_definition_node);
  if (errors_.empty())
  {
    if (!function_->return_type->IsVoid())
    {
      if (TestBlock(function_definition_node))
      {
        AddError(function_definition_node->block_terminator_line,
                 "control reaches end of function without returning a value");
      }
    }
  }
  function_     = nullptr;
  use_any_node_ = nullptr;
}

void Analyser::AnnotateWhileStatement(BlockNodePtr const &while_statement_node)
{
  AnnotateConditionalBlock(while_statement_node);
}

void Analyser::AnnotateForStatement(BlockNodePtr const &for_statement_node)
{
  ExpressionNodePtr  name_node = ConvertToExpressionNodePtr(for_statement_node->children[0]);
  std::string const &name      = name_node->text;
  // Note: variable is created with no type to mark as initially unresolved
  VariablePtr variable = CreateVariable(VariableKind::For, name, nullptr);
  for_statement_node->symbols->Add(variable);
  name_node->variable                  = variable;
  std::size_t const              count = for_statement_node->children.size() - 1;
  std::vector<ExpressionNodePtr> nodes;
  int                            problems = 0;
  for (std::size_t i = 1; i <= count; ++i)
  {
    NodePtr const &   child      = for_statement_node->children[i];
    ExpressionNodePtr child_node = ConvertToExpressionNodePtr(child);
    if (!AnnotateExpression(child_node))
    {
      ++problems;
      continue;
    }
    if (!MatchType(child_node->type, any_integer_type_))
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
    name_node->variable->type          = inferred_for_variable_type;
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

void Analyser::AnnotateUseStatement(BlockNodePtr const &parent_block_node,
                                    NodePtr const &     use_statement_node)
{
  ExpressionNodePtr state_name_node = ConvertToExpressionNodePtr(use_statement_node->children[0]);
  NodePtr           list_node       = use_statement_node->children[1];
  ExpressionNodePtr alias_name_node = ConvertToExpressionNodePtr(use_statement_node->children[2]);
  ExpressionNodePtr variable_name_node = alias_name_node ? alias_name_node : state_name_node;
  TypePtr           type               = state_definitions_.Find(state_name_node->text);
  if (!type)
  {
    AddError(state_name_node->line,
             "unable to find state definition for '" + state_name_node->text + "'");
    return;
  }
  if (list_node)
  {
    if (type->template_type != sharded_state_type_)
    {
      AddError(list_node->line, "key list can only be used with a sharded state");
      return;
    }
    for (auto const &c : list_node->children)
    {
      ExpressionNodePtr child = ConvertToExpressionNodePtr(c);
      if (!AnnotateExpression(child))
      {
        return;
      }
      if ((child->type != string_type_) && (child->type != address_type_))
      {
        AddError(child->line, "key must be String or Address type");
        return;
      }
    }
  }
  std::string variable_name = variable_name_node->text;
  SymbolPtr   symbol        = parent_block_node->symbols->Find(variable_name);
  if (symbol)
  {
    AddError(variable_name_node->line, "symbol '" + variable_name + "' is already defined");
    return;
  }
  VariablePtr variable = CreateVariable(VariableKind::Use, variable_name, type);
  parent_block_node->symbols->Add(variable);
  FunctionPtr constructor = (type->template_type == sharded_state_type_)
                                ? sharded_state_constructor_
                                : state_constructor_;
  variable_name_node->variable = variable;
  variable_name_node->function = constructor;
}

void Analyser::AnnotateUseAnyStatement(BlockNodePtr const &parent_block_node,
                                       NodePtr const &     use_any_statement_node)
{
  if (use_any_node_)
  {
    AddError(use_any_statement_node->line, "duplicate use-any statement");
    return;
  }
  use_any_node_ = use_any_statement_node;
  for (auto &it : state_definitions_.map)
  {
    std::string const &name   = it.first;
    TypePtr const &    type   = it.second;
    SymbolPtr          symbol = parent_block_node->symbols->Find(name);
    if (symbol)
    {
      AddError(use_any_statement_node->line, "symbol '" + name + "' is already defined");
      return;
    }
    VariablePtr variable = CreateVariable(VariableKind::UseAny, name, type);
    parent_block_node->symbols->Add(variable);
  }
}

void Analyser::AnnotateContractStatement(BlockNodePtr const &parent_block_node,
                                         NodePtr const &     contract_statement_node)
{
  ExpressionNodePtr contract_variable_node =
      ConvertToExpressionNodePtr(contract_statement_node->children[0]);
  ExpressionNodePtr contract_type_node =
      ConvertToExpressionNodePtr(contract_statement_node->children[1]);
  ExpressionNodePtr initialiser_node =
      ConvertToExpressionNodePtr(contract_statement_node->children[2]);

  std::string const &contract_variable_name = contract_variable_node->text;
  std::string const &contract_type_name     = contract_type_node->text;

  if (parent_block_node->symbols->Find(contract_variable_name))
  {
    AddError(contract_variable_node->line,
             "symbol '" + contract_variable_name + "' is already defined");
    return;
  }
  TypePtr contract_type = contract_definitions_.Find(contract_type_name);
  if (!contract_type)
  {
    AddError(contract_type_node->line, "unknown contract '" + contract_type_name + "'");
    return;
  }
  if (!AnnotateExpression(initialiser_node))
  {
    return;
  }
  if (initialiser_node->type != string_type_)
  {
    AddError(initialiser_node->line, "contract initialiser must be String type");
    return;
  }

  VariablePtr variable = CreateVariable(VariableKind::Var, contract_variable_name, contract_type);
  parent_block_node->symbols->Add(variable);
  contract_variable_node->variable = variable;
  contract_type_node->type         = contract_type;
}

void Analyser::AnnotateVarStatement(BlockNodePtr const &parent_block_node,
                                    NodePtr const &     var_statement_node)
{
  ExpressionNodePtr  name_node = ConvertToExpressionNodePtr(var_statement_node->children[0]);
  std::string const &name      = name_node->text;
  SymbolPtr          symbol    = parent_block_node->symbols->Find(name);
  if (symbol)
  {
    AddError(name_node->line, "symbol '" + name + "' is already defined");
    return;
  }

  // Note: variable is created with no type to mark as initially unresolved
  VariablePtr variable = CreateVariable(VariableKind::Var, name, nullptr);
  parent_block_node->symbols->Add(variable);
  name_node->variable = variable;

  if (var_statement_node->node_kind == NodeKind::VarDeclarationStatement)
  {
    ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (!AnnotateTypeExpression(type_node))
    {
      return;
    }
    variable->type = type_node->type;
  }
  else if (var_statement_node->node_kind == NodeKind::VarDeclarationTypedAssignmentStatement)
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
    if (expression_node->node_kind == NodeKind::InitialiserList)
    {
      if (!ConvertInitialiserList(expression_node, type_node->type))
      {
        AddError(type_node->line, "incompatible types");
        return;
      }
    }
    else if (!expression_node->type->IsNull())
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
    if (!AnnotateExpression(expression_node))
    {
      return;
    }
    if (expression_node->type->IsVoid() || expression_node->type->IsNull() ||
        expression_node->node_kind == NodeKind::InitialiserList)
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
    if (!AnnotateExpression(expression_node))
    {
      return;
    }
    if (expression_node->node_kind == NodeKind::InitialiserList)
    {
      if (!ConvertInitialiserList(expression_node, function_->return_type))
      {
        AddError(expression_node->line, "incompatible types");
        return;
      }
    }
    else if (!expression_node->type->IsNull())
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
    if (!function_->return_type->IsVoid())
    {
      AddError(return_statement_node->line, "return does not supply a value");
      return;
    }
  }
}

void Analyser::AnnotateConditionalBlock(BlockNodePtr const &conditional_block_node)
{
  ExpressionNodePtr expression_node =
      ConvertToExpressionNodePtr(conditional_block_node->children[0]);
  if (AnnotateExpression(expression_node))
  {
    if (expression_node->type != bool_type_)
    {
      AddError(expression_node->line, "boolean type expected");
    }
  }
  AnnotateBlock(conditional_block_node);
}

bool Analyser::AnnotateTypeExpression(ExpressionNodePtr const &node)
{
  TypePtr type = FindType(node);
  if (!type)
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
  if (!AnnotateLHSExpression(node, lhs) || !AnnotateExpression(rhs))
  {
    return false;
  }

  if (rhs->node_kind == NodeKind::InitialiserList)
  {
    if (!ConvertInitialiserList(rhs, lhs->type))
    {
      AddError(node->line, "incompatible types");
      return false;
    }
  }
  else if (rhs->type->IsVoid())
  {
    // Can't assign from a function with no return value
    AddError(node->line, "incompatible types");
    return false;
  }
  else if (!rhs->type->IsNull())
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
  if (!AnnotateLHSExpression(node, lhs) || !AnnotateExpression(rhs))
  {
    return false;
  }
  return AnnotateArithmetic(node, lhs, rhs);
}

bool Analyser::AnnotateInplaceModuloOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (!AnnotateLHSExpression(node, lhs) || !AnnotateExpression(rhs))
  {
    return false;
  }
  if (lhs->type != rhs->type || !MatchType(lhs->type, any_integer_type_))
  {
    AddError(node->line, "integral operands expected");
    return false;
  }
  SetRVExpression(node, lhs->type);
  return true;
}

bool Analyser::AnnotateLHSExpression(ExpressionNodePtr const &node, ExpressionNodePtr const &lhs)
{
  if (!AnnotateExpression(lhs) || !IsWriteable(lhs))
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
  SymbolTablePtr    symbols;
  if (container_node->type->IsInstantiation())
  {
    symbols = container_node->type->template_type->symbols;
  }
  else
  {
    symbols = container_node->type->symbols;
  }

  std::size_t const      num_supplied_indexes = lhs->children.size() - 1;
  ExpressionNodePtrArray supplied_index_nodes;
  for (std::size_t i = 1; i <= num_supplied_indexes; ++i)
  {
    NodePtr const &   supplied_index      = lhs->children[i];
    ExpressionNodePtr supplied_index_node = ConvertToExpressionNodePtr(supplied_index);
    supplied_index_nodes.push_back(supplied_index_node);
  }

  SymbolPtr setter_symbol = symbols->Find(SET_INDEXED_VALUE);
  if (!setter_symbol)
  {
    AddError(container_node->line, "unable to find matching index operator for type '" +
                                       container_node->type->name + "'");
    return false;
  }

  FunctionGroupPtr setter_fg = ConvertToFunctionGroupPtr(setter_symbol);

  TypePtrArray           dummy;
  ExpressionNodePtrArray setter_supplied_nodes = supplied_index_nodes;
  setter_supplied_nodes.push_back(lhs);
  FunctionPtr setter_f =
      FindFunction(container_node->type, setter_fg, setter_supplied_nodes, dummy);
  if (!setter_f)
  {
    AddError(container_node->line, "unable to find matching index operator for type '" +
                                       container_node->type->name + "'");
    return false;
  }

  node->function = setter_f;
  return true;
}

bool Analyser::AnnotateExpression(ExpressionNodePtr const &node)
{
  if (!InternalAnnotateExpression(node))
  {
    return false;
  }
  if (node->IsTypeExpression())
  {
    AddError(node->line, "illegal use of type '" + node->type->name + "'");
    return false;
  }
  if (node->IsFunctionGroupExpression())
  {
    AddError(node->line, "illegal use of function '" + node->function_group->name + "'");
    return false;
  }
  // node->IsVariableExpression()
  // node->IsLVExpression()
  // node->IsRVExpression()
  if (node->IsVariableExpression() && node->type->IsUserDefinedContract())
  {
    AddError(node->line, "unable to use contract variable '" + node->variable->name + "'");
    return false;
  }
  return true;
}

bool Analyser::InternalAnnotateExpression(ExpressionNodePtr const &node)
{
  switch (node->node_kind)
  {
  case NodeKind::Identifier:
  case NodeKind::Template:
  {
    SymbolPtr symbol = FindSymbol(node);
    if (!symbol)
    {
      AddError(node->line, "unknown symbol '" + node->text + "'");
      return false;
    }
    if (symbol->IsFunctionGroup())
    {
      // Function name
      FunctionGroupPtr function_group = ConvertToFunctionGroupPtr(symbol);
      SetFunctionGroupExpression(node, function_group, nullptr, false);
    }
    else if (symbol->IsType())
    {
      // Type name
      TypePtr type = ConvertToTypePtr(symbol);
      SetTypeExpression(node, type);
    }
    else
    {
      // Variable name
      VariablePtr variable = ConvertToVariablePtr(symbol);
      if (!variable->type)
      {
        AddError(node->line, "variable '" + node->text + "' has unresolved type");
        return false;
      }
      SetVariableExpression(node, variable);
    }
    break;
  }
  case NodeKind::Integer8:
  {
    SetRVExpression(node, int8_type_);
    break;
  }
  case NodeKind::UnsignedInteger8:
  {
    SetRVExpression(node, uint8_type_);
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
  case NodeKind::Fixed32:
  {
    SetRVExpression(node, fixed32_type_);
    break;
  }
  case NodeKind::Fixed64:
  {
    SetRVExpression(node, fixed64_type_);
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
    if (!AnnotateEqualityOp(node))
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
    if (!AnnotateRelationalOp(node))
    {
      return false;
    }
    break;
  }
  case NodeKind::And:
  case NodeKind::Or:
  {
    if (!AnnotateBinaryLogicalOp(node))
    {
      return false;
    }
    break;
  }
  case NodeKind::Not:
  {
    if (!AnnotateUnaryLogicalOp(node))
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
    if (!AnnotatePrefixPostfixOp(node))
    {
      return false;
    }
    break;
  }
  case NodeKind::Negate:
  {
    if (!AnnotateNegateOp(node))
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
    if (!AnnotateArithmeticOp(node))
    {
      return false;
    }
    break;
  }
  case NodeKind::Modulo:
  {
    if (!AnnotateModuloOp(node))
    {
      return false;
    }
    break;
  }
  case NodeKind::Index:
  {
    if (!AnnotateIndexOp(node))
    {
      return false;
    }
    break;
  }
  case NodeKind::Dot:
  {
    if (!AnnotateDotOp(node))
    {
      return false;
    }
    break;
  }
  case NodeKind::Invoke:
  {
    if (!AnnotateInvokeOp(node))
    {
      return false;
    }
    break;
  }
  case NodeKind::InitialiserList:
  {
    if (!AnnotateInitialiserList(node))
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
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
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
      if (rhs->node_kind == NodeKind::InitialiserList)
      {
        if (!ConvertInitialiserList(rhs, lhs->type))
        {
          AddError(node->line, "incompatible types");
          return false;
        }
      }
      else if (lhs->node_kind == NodeKind::InitialiserList)
      {
        if (!ConvertInitialiserList(lhs, rhs->type))
        {
          AddError(node->line, "incompatible types");
          return false;
        }
      }

      if (lhs->type != rhs->type)
      {
        AddError(node->line, "incompatible types");
        return false;
      }
      bool const enabled = IsOperatorEnabled(lhs->type, op);
      if (!enabled)
      {
        AddError(node->line, "operator not supported");
        return false;
      }
    }
    else
    {
      if (lhs_is_primitive && lhs->node_kind != NodeKind::InitialiserList)
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
      if (rhs_is_primitive && rhs->node_kind != NodeKind::InitialiserList)
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
      // Type-uninferable nulls will be transformed to boolean false
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
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (rhs->node_kind == NodeKind::InitialiserList)
  {
    if (!ConvertInitialiserList(rhs, lhs->type))
    {
      AddError(node->line, "incompatible types");
      return false;
    }
  }
  else if (lhs->node_kind == NodeKind::InitialiserList)
  {
    if (!ConvertInitialiserList(lhs, rhs->type))
    {
      AddError(node->line, "incompatible types");
      return false;
    }
  }
  if (lhs->type != rhs->type)
  {
    AddError(node->line, "incompatible types");
    return false;
  }
  if (!IsOperatorEnabled(lhs->type, op))
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
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
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
  if (!AnnotateExpression(operand))
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
  if (!AnnotateLHSExpression(node, operand))
  {
    return false;
  }
  if (!MatchType(operand->type, any_integer_type_))
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
  if (!AnnotateExpression(operand))
  {
    return false;
  }
  if (!IsOperatorEnabled(operand->type, op))
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
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
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
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs = ConvertToExpressionNodePtr(node->children[1]);
  if (lhs->type != rhs->type || !MatchType(lhs->type, any_integer_type_))
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
  if (!InternalAnnotateExpression(lhs))
  {
    return false;
  }
  if (lhs->IsTypeExpression() || lhs->IsFunctionGroupExpression())
  {
    AddError(lhs->line, "operand does not support index operator");
    return false;
  }
  // lhs->IsVariableExpression()
  // lhs->IsLVExpression()
  // lhs->IsRVExpression()
  if (lhs->IsVariableExpression() && lhs->type->IsUserDefinedContract())
  {
    AddError(lhs->line, "unable to use contract variable '" + lhs->variable->name + "'");
    return false;
  }
  if (lhs->node_kind == NodeKind::InitialiserList)
  {
    AddError(lhs->line, "indexing into initialiser lists not supported");
    return false;
  }
  if (lhs->type->IsPrimitive())
  {
    AddError(lhs->line, "primitive type '" + lhs->type->name + "' does not support index operator");
    return false;
  }
  SymbolTablePtr symbols;
  if (lhs->type->IsInstantiation())
  {
    symbols = lhs->type->template_type->symbols;
  }
  else
  {
    symbols = lhs->type->symbols;
  }
  std::size_t const      num_supplied_indexes = node->children.size() - 1;
  ExpressionNodePtrArray supplied_index_nodes;
  for (std::size_t i = 1; i <= num_supplied_indexes; ++i)
  {
    NodePtr const &   supplied_index      = node->children[i];
    ExpressionNodePtr supplied_index_node = ConvertToExpressionNodePtr(supplied_index);
    if (!AnnotateExpression(supplied_index_node))
    {
      return false;
    }
    supplied_index_nodes.push_back(supplied_index_node);
  }
  SymbolPtr symbol = symbols->Find(GET_INDEXED_VALUE);
  if (!symbol)
  {
    AddError(lhs->line,
             "unable to find matching index operator for type '" + lhs->type->name + "'");
    return false;
  }
  FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
  TypePtrArray     actual_index_types;
  FunctionPtr      f = FindFunction(lhs->type, fg, supplied_index_nodes, actual_index_types);
  if (!f)
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
  TypePtr output_type = ResolveType(f->return_type, lhs->type);
  SetLVExpression(node, output_type);
  node->function = f;
  return true;
}

bool Analyser::AnnotateDotOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  if (!InternalAnnotateExpression(lhs))
  {
    return false;
  }
  ExpressionNodePtr  rhs         = ConvertToExpressionNodePtr(node->children[1]);
  std::string const &member_name = rhs->text;
  if (lhs->IsFunctionGroupExpression())
  {
    AddError(lhs->line, "operand does not support member-access operator");
    return false;
  }
  // lhs->IsVariableExpression()
  // lhs->IsLVExpression()
  // lhs->IsRVExpression()
  // lhs->IsTypeExpression()
  bool lhs_is_type_expression = lhs->IsTypeExpression();
  if (lhs->type->IsPrimitive())
  {
    AddError(lhs->line,
             "primitive type '" + lhs->type->name + "' does not support member-access operator");
    return false;
  }
  SymbolTablePtr symbols;
  if (lhs->type->IsInstantiation())
  {
    symbols = lhs->type->template_type->symbols;
  }
  else
  {
    symbols = lhs->type->symbols;
  }
  SymbolPtr member_symbol = symbols->Find(member_name);
  if (!member_symbol)
  {
    AddError(lhs->line, "'" + lhs->type->name + "' has no member named '" + member_name + "'");
    return false;
  }
  if (member_symbol->IsFunctionGroup())
  {
    // member is a function name
    FunctionGroupPtr function_group = ConvertToFunctionGroupPtr(member_symbol);
    SetFunctionGroupExpression(node, function_group, lhs->type, !lhs_is_type_expression);
    return true;
  }
  if (member_symbol->IsType())
  {
    // member is a type name
    AddError(lhs->line, "not supported");
    return false;
  }
  // member is a variable name
  AddError(lhs->line, "not supported");
  return false;
}

bool Analyser::AnnotateInvokeOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  if (!InternalAnnotateExpression(lhs))
  {
    return false;
  }
  ExpressionNodePtrArray supplied_parameter_nodes;
  for (std::size_t i = 1; i < node->children.size(); ++i)
  {
    NodePtr const &   supplied_parameter      = node->children[i];
    ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
    if (!AnnotateExpression(supplied_parameter_node))
    {
      return false;
    }
    supplied_parameter_nodes.push_back(supplied_parameter_node);
  }
  if (lhs->IsFunctionGroupExpression())
  {
    // Note that lhs->type is null for free functions
    TypePtrArray actual_parameter_types;
    FunctionPtr  f = FindFunction(lhs->type, lhs->function_group, supplied_parameter_nodes,
                                 actual_parameter_types);
    if (!f)
    {
      // No matching function, or ambiguous
      AddError(lhs->line,
               "unable to find matching function for '" + lhs->function_group->name + "'");
      return false;
    }
    if (f->function_kind == FunctionKind::StaticMemberFunction)
    {
      if (lhs->function_invoker_is_instance)
      {
        AddError(lhs->line,
                 "function '" + lhs->function_group->name + "' is a static member function");
        return false;
      }
    }
    else if (f->function_kind == FunctionKind::MemberFunction)
    {
      if (!lhs->function_invoker_is_instance)
      {
        AddError(lhs->line,
                 "function '" + lhs->function_group->name + "' is a non-static member function");
        return false;
      }
    }
    for (std::size_t i = 1; i < node->children.size(); ++i)
    {
      NodePtr const &   supplied_parameter      = node->children[i];
      ExpressionNodePtr supplied_parameter_node = ConvertToExpressionNodePtr(supplied_parameter);
      supplied_parameter_node->type             = actual_parameter_types[i - 1];
    }
    TypePtr return_type = ResolveType(f->return_type, lhs->type);
    SetRVExpression(node, return_type);
    node->function = f;
    return true;
  }
  if (lhs->IsTypeExpression())
  {
    // Type constructor
    if (lhs->type->IsPrimitive())
    {
      AddError(lhs->line, "primitive type '" + lhs->type->name + "' is not constructible");
      return false;
    }
    SymbolTablePtr symbols;
    if (lhs->type->IsInstantiation())
    {
      symbols = lhs->type->template_type->symbols;
    }
    else
    {
      symbols = lhs->type->symbols;
    }
    SymbolPtr symbol = symbols->Find(CONSTRUCTOR);
    if (!symbol)
    {
      AddError(lhs->line, "unable to find matching constructor for type '" + lhs->type->name + "'");
      return false;
    }
    FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
    TypePtrArray     actual_parameter_types;
    FunctionPtr f = FindFunction(lhs->type, fg, supplied_parameter_nodes, actual_parameter_types);
    if (!f)
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
  // lhs->IsVariableExpression()
  // lhs->IsLVExpression()
  // lhs->IsRVExpression()
  // e.g.
  // variable();
  // (a + b)();
  // array[index]();
  AddError(lhs->line, "operand does not support function-call operator");
  return false;
}

bool Analyser::AnnotateInitialiserList(ExpressionNodePtr const &node)
{
  for (NodePtr const &child : node->children)
  {
    if (!AnnotateExpression(ConvertToExpressionNodePtr(child)))
    {
      return false;
    }
  }
  SetRVExpression(node, initialiser_list_type_);
  return true;
}

bool Analyser::ConvertInitialiserList(ExpressionNodePtr const &node, TypePtr const &type)
{
  return type->IsInstantiation() && (type->template_type == array_type_) &&
         ConvertInitialiserListToArray(node, type);
}

bool Analyser::ConvertInitialiserListToArray(ExpressionNodePtr const &node, TypePtr const &type)
{
  node->type                          = initialiser_list_type_;
  TypePtr const &element_type         = type->types[0];
  bool           element_is_primitive = element_type->IsPrimitive();
  for (NodePtr const &c : node->children)
  {
    auto child = ConvertToExpressionNodePtr(c);
    switch (child->node_kind)
    {
    case NodeKind::InitialiserList:
      if (!ConvertInitialiserList(child, element_type))
      {
        return false;
      }
      break;

    case NodeKind::Null:
      if (element_is_primitive)
      {
        child->type = null_type_;
        return false;
      }
      child->type = element_type;
      break;

    default:
      if (child->type->IsVoid() || child->type != element_type)
      {
        return false;
      }
    }
  }
  // all children were convertible to the element type, so set the type
  node->type = type;
  return true;
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
        if (!able_to_reach_end_of_if_statement)
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

bool Analyser::IsWriteable(ExpressionNodePtr const &node)
{
  bool const is_variable_expression = node->IsVariableExpression();
  bool const is_lv_expression       = node->IsLVExpression();
  if (!is_variable_expression && !is_lv_expression)
  {
    AddError(node->line, "assignment operand is not writeable");
    return false;
  }
  if (is_variable_expression)
  {
    if ((node->variable->variable_kind == VariableKind::Parameter) ||
        (node->variable->variable_kind == VariableKind::For))
    {
      AddError(node->line, "assignment operand is not writeable");
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
      if (rhs_is_instantiation && IsLeftOperatorEnabled(rhs->type, op))
      {
        if (lhs->node_kind == NodeKind::InitialiserList)
        {
          if (!ConvertInitialiserList(lhs, rhs->type))
          {
            AddError(node->line, "incompatible types");
            return false;
          }
        }
        else
        {
          // primitive op object
          TypePtr const &rhs_type = rhs->type->types[0];
          if (lhs->type == rhs_type)
          {
            SetRVExpression(node, rhs->type);
            return true;
          }
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
        if (rhs->node_kind == NodeKind::InitialiserList)
        {
          if (!ConvertInitialiserList(rhs, lhs->type))
          {
            AddError(node->line, "incompatible types");
            return false;
          }
        }
        else
        {
          TypePtr const &lhs_type = lhs->type->types[0];
          if (lhs_type == rhs->type)
          {
            SetRVExpression(node, lhs->type);
            return true;
          }
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

TypePtr Analyser::ResolveType(TypePtr const &type, TypePtr const &instantiation_type)
{
  if (type->type_kind == TypeKind::Template)
  {
    return instantiation_type;
  }
  if (type == template_parameter1_type_)
  {
    return instantiation_type->types[0];
  }
  if (type == template_parameter2_type_)
  {
    return instantiation_type->types[1];
  }
  return type;
}

bool Analyser::MatchType(TypePtr const &supplied_type, TypePtr const &expected_type) const
{
  if (expected_type == any_type_)
  {
    return true;
  }
  if (expected_type == any_primitive_type_)
  {
    return supplied_type->IsPrimitive();
  }
  if (expected_type->IsGroup())
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

bool Analyser::MatchTypes(TypePtr const &type, ExpressionNodePtrArray const &supplied_nodes,
                          TypePtrArray const &expected_types, TypePtrArray &actual_types)
{
  actual_types.clear();
  std::size_t const num_types = expected_types.size();
  if (supplied_nodes.size() != num_types)
  {
    // Not a match
    return false;
  }
  for (std::size_t i = 0; i < num_types; ++i)
  {
    auto const &   supplied_node = supplied_nodes[i];
    TypePtr const &supplied_type = supplied_node->type;
    if (supplied_type->IsVoid())
    {
      return false;
    }
    TypePtr expected_type = ResolveType(expected_types[i], type);
    if (supplied_type->IsNull())
    {
      if (!expected_type->IsClass() && !expected_type->IsInstantiation())
      {
        // Not a match, can only convert null to a known reference type
        return false;
      }
      actual_types.push_back(expected_type);
    }
    else if (supplied_node->node_kind == NodeKind::InitialiserList)
    {
      // if there's a matching type set, this node's type will be properly set in the end
      // otherwise, the whole script does not compile
      // so it does not matter if we change node's type now
      if (!ConvertInitialiserList(supplied_node, expected_type))
      {
        return false;
      }
      actual_types.push_back(expected_type);
    }
    else
    {
      if (!MatchType(supplied_type, expected_type))
      {
        return false;
      }
      actual_types.push_back(supplied_type);
    }
  }
  // Got a match
  return true;
}

FunctionPtr Analyser::FindFunction(TypePtr const &type, FunctionGroupPtr const &function_group,
                                   ExpressionNodePtrArray const &supplied_nodes,
                                   TypePtrArray &                actual_types)
{
  FunctionPtrArray          functions;
  std::vector<TypePtrArray> array;
  for (FunctionPtr const &function : function_group->functions)
  {
    TypePtrArray temp_actual_types;
    if (MatchTypes(type, supplied_nodes, function->parameter_types, temp_actual_types))
    {
      array.push_back(temp_actual_types);
      functions.push_back(function);
    }
  }
  if (functions.size() == 1)
  {
    actual_types = array[0];
    for (std::size_t i{}; i < actual_types.size(); ++i)
    {
      auto const &supplied_node = supplied_nodes[i];
      if (supplied_node->node_kind == NodeKind::InitialiserList)
      {
        ConvertInitialiserList(supplied_node, actual_types[i]);
      }
    }
    return functions[0];
  }
  // Matching function not found, or ambiguous
  return nullptr;
}

TypePtr Analyser::FindType(ExpressionNodePtr const &node)
{
  SymbolPtr symbol = FindSymbol(node);
  if (symbol && symbol->IsType())
  {
    return ConvertToTypePtr(symbol);
  }
  return {};
}

SymbolPtr Analyser::FindSymbol(ExpressionNodePtr const &node)
{
  if (node->node_kind == NodeKind::Template)
  {
    SymbolPtr symbol = SearchSymbols(node->text);
    if (symbol)
    {
      // Template instantiation already exists
      return ConvertToTypePtr(symbol);
    }
    ExpressionNodePtr  identifier_node = ConvertToExpressionNodePtr(node->children[0]);
    std::string const &name            = identifier_node->text;
    symbol                             = SearchSymbols(name);
    if (!symbol)
    {
      // Template type doesn't exist
      return nullptr;
    }
    TypePtr           template_type                = ConvertToTypePtr(symbol);
    std::size_t const num_supplied_parameter_types = node->children.size() - 1;
    std::size_t const num_expected_parameter_types = template_type->types.size();
    if (num_supplied_parameter_types != num_expected_parameter_types)
    {
      return nullptr;
    }
    TypePtrArray template_parameter_types;
    for (std::size_t i = 1; i <= num_expected_parameter_types; ++i)
    {
      ExpressionNodePtr parameter_type_node = ConvertToExpressionNodePtr(node->children[i]);
      TypePtr           parameter_type      = FindType(parameter_type_node);
      if (!parameter_type)
      {
        return nullptr;
      }
      TypePtr const &expected_parameter_type = template_type->types[i - 1];
      if (!MatchType(parameter_type, expected_parameter_type))
      {
        return nullptr;
      }
      // Need to check here that parameter_type does in fact support any operator(s)
      // required by the template_type's i'th type parameter...
      template_parameter_types.push_back(std::move(parameter_type));
    }
    TypePtr type = InternalCreateTemplateInstantiationType(
        TypeKind::UserDefinedTemplateInstantiation, template_type, template_parameter_types);
    root_->symbols->Add(type);
    return type;
  }

  return SearchSymbols(node->text);
}

SymbolPtr Analyser::SearchSymbols(std::string const &name)
{
  SymbolPtr symbol = symbols_->Find(name);
  if (symbol)
  {
    return symbol;
  }
  for (auto it = blocks_.rbegin(); it != blocks_.rend(); ++it)
  {
    BlockNodePtr const &block_node = *it;
    symbol                         = block_node->symbols->Find(name);
    if (symbol)
    {
      return symbol;
    }
  }
  // Name not found in any symbol table
  return nullptr;
}

void Analyser::SetVariableExpression(ExpressionNodePtr const &node, VariablePtr const &variable)
{
  node->expression_kind = ExpressionKind::Variable;
  node->variable        = variable;
  node->type            = variable->type;
  variable->referenced  = true;
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

// free function:     function_invoker_type is null      function_invoker_is_instance is false
// instance function: function_invoker_type is non-null  function_invoker_is_instance is true
// type function:     function_invoker_type is non-null  function_invoker_is_instance is false
void Analyser::SetFunctionGroupExpression(ExpressionNodePtr const &node,
                                          FunctionGroupPtr const & function_group,
                                          TypePtr const &          function_invoker_type,
                                          bool                     function_invoker_is_instance)
{
  node->expression_kind              = ExpressionKind::FunctionGroup;
  node->function_group               = function_group;
  node->type                         = function_invoker_type;
  node->function_invoker_is_instance = function_invoker_is_instance;
}

bool Analyser::CheckType(std::string const &type_name, TypeIndex type_index)
{
  TypePtr found_type = type_map_.Find(type_index);
  if (found_type)
  {
    if (found_type->name != type_name)
    {
      throw std::runtime_error("type index " + std::string(type_index.name()) +
                               " has already been registered with a different name");
    }
    // Already created
    return true;
  }
  if (type_set_.Find(type_name))
  {
    throw std::runtime_error("type name '" + type_name + "' has already been registered");
  }
  type_set_.Add(type_name);
  // Not already created
  return false;
}

void Analyser::CreatePrimitiveType(std::string const &type_name, TypeIndex type_index,
                                   bool add_to_symbol_table, TypeId type_id, TypePtr &type)
{
  if (CheckType(type_name, type_index))
  {
    // Already created
    return;
  }
  type = CreateType(TypeKind::Primitive, type_name);
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeKind::Primitive, type_name, type_id, TypeIds::Unknown, {}, type);
  registered_types_.Add(type_index, type->id);
  if (add_to_symbol_table)
  {
    symbols_->Add(type);
  }
}

void Analyser::CreateMetaType(std::string const &type_name, TypeIndex type_index, TypeId type_id,
                              TypePtr &type)
{
  if (CheckType(type_name, type_index))
  {
    // Already created
    return;
  }
  type = CreateType(TypeKind::Meta, type_name);
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeKind::Meta, type_name, type_id, TypeIds::Unknown, {}, type);
  registered_types_.Add(type_index, type->id);
}

void Analyser::CreateClassType(std::string const &type_name, TypeIndex type_index, TypeId type_id,
                               TypePtr &type)
{
  if (CheckType(type_name, type_index))
  {
    // Already created
    return;
  }
  type          = CreateType(TypeKind::Class, type_name);
  type->symbols = CreateSymbolTable();
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeKind::Class, type_name, type_id, TypeIds::Unknown, {}, type);
  registered_types_.Add(type_index, type->id);
  symbols_->Add(type);
}

void Analyser::CreateTemplateType(std::string const &type_name, TypeIndex type_index,
                                  TypePtrArray const &allowed_types, TypeId type_id, TypePtr &type)
{
  if (CheckType(type_name, type_index))
  {
    // Already created
    return;
  }
  type          = CreateType(TypeKind::Template, type_name);
  type->symbols = CreateSymbolTable();
  type->types   = allowed_types;
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeKind::Template, type_name, type_id, TypeIds::Unknown, {}, type);
  registered_types_.Add(type_index, type->id);
  symbols_->Add(type);
}

void Analyser::CreateTemplateInstantiationType(TypeIndex type_index, TypePtr const &template_type,
                                               TypePtrArray const &template_parameter_types,
                                               TypeId type_id, TypePtr &type)
{
  type = InternalCreateTemplateInstantiationType(TypeKind::TemplateInstantiation, template_type,
                                                 template_parameter_types);
  if (CheckType(type->name, type_index))
  {
    // Already created
    return;
  }
  TypeIdArray template_parameter_type_ids;
  for (auto const &template_parameter_type : template_parameter_types)
  {
    template_parameter_type_ids.push_back(template_parameter_type->id);
  }
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeKind::TemplateInstantiation, type->name, type_id, template_type->id,
              template_parameter_type_ids, type);
  registered_types_.Add(type_index, type->id);
  symbols_->Add(type);
}

void Analyser::CreateGroupType(std::string const &type_name, TypeIndex type_index,
                               TypePtrArray const &allowed_types, TypeId type_id, TypePtr &type)
{
  if (CheckType(type_name, type_index))
  {
    // Already created
    return;
  }
  type        = CreateType(TypeKind::Group, type_name);
  type->types = allowed_types;
  type_map_.Add(type_index, type);
  AddTypeInfo(TypeKind::Group, type_name, type_id, TypeIds::Unknown, {}, type);
  registered_types_.Add(type_index, type->id);
}

TypePtr Analyser::InternalCreateTemplateInstantiationType(
    TypeKind type_kind, TypePtr const &template_type, TypePtrArray const &template_parameter_types)
{
  std::stringstream stream;
  stream << template_type->name + "<";
  std::size_t const count = template_parameter_types.size();
  for (std::size_t i = 0; i < count; ++i)
  {
    stream << template_parameter_types[i]->name;
    if (i + 1 < count)
    {
      stream << ",";
    }
  }
  stream << ">";
  std::string name    = stream.str();
  TypePtr     type    = CreateType(type_kind, name);
  type->template_type = template_type;
  type->types         = template_parameter_types;
  return type;
}

void Analyser::CreateFreeFunction(std::string const &name, TypePtrArray const &parameter_types,
                                  TypePtr const &return_type, Handler const &handler,
                                  ChargeAmount static_charge)
{
  std::string unique_name = BuildUniqueName(nullptr, name, parameter_types, return_type);
  if (function_map_.Find(unique_name))
  {
    // Already created
    return;
  }
  FunctionPtr f = CreateFunction(FunctionKind::FreeFunction, name, unique_name, parameter_types,
                                 VariablePtrArray(), return_type);
  AddFunctionToSymbolTable(symbols_, f);
  AddFunctionInfo(f, handler, static_charge);
  function_map_.Add(f);
}

void Analyser::CreateConstructor(TypePtr const &type, TypePtrArray const &parameter_types,
                                 Handler const &handler, ChargeAmount static_charge)
{
  std::string unique_name = BuildUniqueName(type, CONSTRUCTOR, parameter_types, type);
  if (function_map_.Find(unique_name))
  {
    // Already created
    return;
  }
  FunctionPtr f = CreateFunction(FunctionKind::ConstructorFunction, CONSTRUCTOR, unique_name,
                                 parameter_types, VariablePtrArray(), type);
  AddFunctionToSymbolTable(type->symbols, f);
  AddFunctionInfo(f, handler, static_charge);
  function_map_.Add(f);
}

void Analyser::CreateStaticMemberFunction(TypePtr const &type, std::string const &name,
                                          TypePtrArray const &parameter_types,
                                          TypePtr const &return_type, Handler const &handler,
                                          ChargeAmount static_charge)
{
  std::string unique_name = BuildUniqueName(type, name, parameter_types, return_type);
  if (function_map_.Find(unique_name))
  {
    // Already created
    return;
  }
  FunctionPtr f = CreateFunction(FunctionKind::StaticMemberFunction, name, unique_name,
                                 parameter_types, VariablePtrArray(), return_type);
  AddFunctionToSymbolTable(type->symbols, f);
  AddFunctionInfo(f, handler, static_charge);
  function_map_.Add(f);
}

void Analyser::CreateMemberFunction(TypePtr const &type, std::string const &name,
                                    TypePtrArray const &parameter_types, TypePtr const &return_type,
                                    Handler const &handler, ChargeAmount static_charge)
{
  std::string unique_name = BuildUniqueName(type, name, parameter_types, return_type);
  if (function_map_.Find(unique_name))
  {
    // Already created
    return;
  }
  FunctionPtr f = CreateFunction(FunctionKind::MemberFunction, name, unique_name, parameter_types,
                                 VariablePtrArray(), return_type);
  AddFunctionToSymbolTable(type->symbols, f);
  AddFunctionInfo(f, handler, static_charge);
  function_map_.Add(f);
}

FunctionPtr Analyser::CreateUserDefinedFreeFunction(std::string const &     name,
                                                    TypePtrArray const &    parameter_types,
                                                    VariablePtrArray const &parameter_variables,
                                                    TypePtr const &         return_type)
{
  std::string unique_name = BuildUniqueName(nullptr, name, parameter_types, return_type);
  return CreateFunction(FunctionKind::UserDefinedFreeFunction, name, unique_name, parameter_types,
                        parameter_variables, return_type);
}

FunctionPtr Analyser::CreateUserDefinedContractFunction(TypePtr const &         type,
                                                        std::string const &     name,
                                                        TypePtrArray const &    parameter_types,
                                                        VariablePtrArray const &parameter_variables,
                                                        TypePtr const &         return_type)
{
  std::string unique_name = BuildUniqueName(type, name, parameter_types, return_type);
  return CreateFunction(FunctionKind::UserDefinedContractFunction, name, unique_name,
                        parameter_types, parameter_variables, return_type);
}

void Analyser::EnableIndexOperator(TypePtr const &type, TypePtrArray const &input_types,
                                   TypePtr const &output_type, Handler const &get_handler,
                                   Handler const &set_handler, ChargeAmount get_static_charge,
                                   ChargeAmount set_static_charge)
{
  std::string g_unique_name = BuildUniqueName(type, GET_INDEXED_VALUE, input_types, output_type);
  if (function_map_.Find(g_unique_name))
  {
    return;
  }

  TypePtrArray s_input_types = input_types;
  s_input_types.push_back(output_type);
  std::string s_unique_name = BuildUniqueName(type, SET_INDEXED_VALUE, s_input_types, void_type_);
  if (function_map_.Find(s_unique_name))
  {
    return;
  }

  FunctionPtr gf = CreateFunction(FunctionKind::MemberFunction, GET_INDEXED_VALUE, g_unique_name,
                                  input_types, VariablePtrArray(), output_type);
  AddFunctionInfo(gf, get_handler, get_static_charge);
  AddFunctionToSymbolTable(type->symbols, gf);
  function_map_.Add(gf);

  FunctionPtr sf = CreateFunction(FunctionKind::MemberFunction, SET_INDEXED_VALUE, s_unique_name,
                                  s_input_types, VariablePtrArray(), void_type_);
  AddFunctionInfo(sf, set_handler, set_static_charge);
  AddFunctionToSymbolTable(type->symbols, sf);
  function_map_.Add(sf);
}

void Analyser::AddTypeInfo(TypeKind type_kind, std::string const &type_name, TypeId type_id,
                           TypeId template_type_id, TypeIdArray const &template_parameter_type_ids,
                           TypePtr const &type)
{
  TypeId id;
  if (type_id == TypeIds::Unknown)
  {
    id = TypeId(type_info_array_.size());
    TypeInfo info(type_kind, type_name, id, template_type_id, template_parameter_type_ids);
    type_info_array_.push_back(std::move(info));
  }
  else
  {
    id = type_id;
    TypeInfo info(type_kind, type_name, id, template_type_id, template_parameter_type_ids);
    type_info_array_[id] = info;
  }
  type->id                   = id;
  type_info_map_[type->name] = id;
}

void Analyser::AddFunctionInfo(FunctionPtr const &function, Handler const &handler,
                               ChargeAmount static_charge)
{
  function_info_array_.emplace_back(function->function_kind, function->unique_name, handler,
                                    static_charge);
}

std::string Analyser::BuildUniqueName(TypePtr const &type, std::string const &function_name,
                                      TypePtrArray const &parameter_types,
                                      TypePtr const &     return_type)
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
  return stream.str();
}

void Analyser::AddFunctionToSymbolTable(SymbolTablePtr const &symbols, FunctionPtr const &function)
{
  FunctionGroupPtr function_group;
  SymbolPtr        symbol = symbols->Find(function->name);
  if (symbol)
  {
    function_group = ConvertToFunctionGroupPtr(symbol);
  }
  else
  {
    // Create new function group
    function_group = CreateFunctionGroup(function->name);
    symbols->Add(function_group);
  }
  // Add the function to the function group
  function_group->functions.push_back(function);
}

}  // namespace vm
}  // namespace fetch
