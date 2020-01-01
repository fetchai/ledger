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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/analyser.hpp"
#include "vm/array.hpp"
#include "vm/fixed.hpp"
#include "vm/map.hpp"
#include "vm/pair.hpp"
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

std::string const Analyser::CONSTRUCTOR       = "[Constructor]";
std::string const Analyser::GET_INDEXED_VALUE = "[GetIndexedValue]";
std::string const Analyser::SET_INDEXED_VALUE = "[SetIndexedValue]";

namespace {

std::string const INIT_ANNOTATION      = "@init";
std::string const ACTION_ANNOTATION    = "@action";
std::string const QUERY_ANNOTATION     = "@query";
std::string const PROBLEM_ANNOTATION   = "@problem";
std::string const OBJECTIVE_ANNOTATION = "@objective";
std::string const WORK_ANNOTATION      = "@work";
std::string const CLEAR_ANNOTATION     = "@clear";

bool HasAnnotation(NodePtr const &annotations_node, std::string const &annotation_name)
{
  if (annotations_node)
  {
    assert(annotations_node->node_kind == NodeKind::Annotations);

    for (auto const &annotation : annotations_node->children)
    {
      if (annotation->node_kind == NodeKind::Annotation && annotation->text == annotation_name)
      {
        return true;
      }
    }
  }

  return false;
}

}  // namespace

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
  CreatePrimitiveType("Fixed32", TypeIndex(typeid(fixed_point::fp32_t)), true, TypeIds::Fixed32,
                      fixed32_type_);
  CreatePrimitiveType("Fixed64", TypeIndex(typeid(fixed_point::fp64_t)), true, TypeIds::Fixed64,
                      fixed64_type_);

  CreateClassType("Fixed128", TypeIndex(typeid(Fixed128)), TypeIds::Fixed128, fixed128_type_);
  EnableOperator(fixed128_type_, Operator::Equal);
  EnableOperator(fixed128_type_, Operator::NotEqual);
  EnableOperator(fixed128_type_, Operator::LessThan);
  EnableOperator(fixed128_type_, Operator::LessThanOrEqual);
  EnableOperator(fixed128_type_, Operator::GreaterThan);
  EnableOperator(fixed128_type_, Operator::GreaterThanOrEqual);
  EnableOperator(fixed128_type_, Operator::Add);
  EnableOperator(fixed128_type_, Operator::InplaceAdd);
  EnableOperator(fixed128_type_, Operator::Subtract);
  EnableOperator(fixed128_type_, Operator::InplaceSubtract);
  EnableOperator(fixed128_type_, Operator::Multiply);
  EnableOperator(fixed128_type_, Operator::InplaceMultiply);
  EnableOperator(fixed128_type_, Operator::Divide);
  EnableOperator(fixed128_type_, Operator::InplaceDivide);
  EnableOperator(fixed128_type_, Operator::Negate);

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
                                     fixed32_type_, fixed64_type_, fixed128_type_};
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

  CreateTemplateType("Array", TypeIndex(typeid(IArray)), {any_type_}, TypeIds::Unknown,
                     array_type_);
  CreateTemplateType("Map", TypeIndex(typeid(IMap)), {any_type_, any_type_}, TypeIds::Unknown,
                     map_type_);

  CreateTemplateType("Pair", TypeIndex(typeid(IPair)), {any_type_, any_type_}, TypeIds::Unknown,
                     pair_type_);

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
  fixed32_type_             = nullptr;
  fixed64_type_             = nullptr;
  fixed128_type_            = nullptr;
  string_type_              = nullptr;
  address_type_             = nullptr;
  template_parameter1_type_ = nullptr;
  template_parameter2_type_ = nullptr;
  any_type_                 = nullptr;
  any_primitive_type_       = nullptr;
  any_integer_type_         = nullptr;
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
  num_state_definitions_                = 0;
  num_contract_definitions_             = 0;
  num_free_functions_                   = 0;
  num_user_defined_types_               = 0;
  num_user_defined_instantiation_types_ = 0;
  function_                             = nullptr;
  use_any_node_                         = nullptr;
  file_errors_array_.clear();

  root_->symbols = CreateSymbolTable();

  try
  {
    BuildBlock(root_);
    PreAnnotateBlock(root_);
    EnforceLedgerRestrictions(root_);
    if (!file_errors_array_.empty())
    {
      errors = GetErrorList();
      root_  = nullptr;
      blocks_.clear();
      loops_.clear();
      state_constructor_         = nullptr;
      sharded_state_constructor_ = nullptr;
      state_definitions_.Clear();
      contract_definitions_.Clear();
      num_state_definitions_                = 0;
      num_contract_definitions_             = 0;
      num_free_functions_                   = 0;
      num_user_defined_types_               = 0;
      num_user_defined_instantiation_types_ = 0;
      function_                             = nullptr;
      use_any_node_                         = nullptr;
      file_errors_array_.clear();
      return false;
    }
    AnnotateBlock(root_);
  }
  catch (FatalErrorException const &e)
  {
    AddError(e.filename, e.line, e.message);
  }

  root_ = nullptr;
  blocks_.clear();
  loops_.clear();
  state_constructor_         = nullptr;
  sharded_state_constructor_ = nullptr;
  state_definitions_.Clear();
  contract_definitions_.Clear();
  num_state_definitions_                = 0;
  num_contract_definitions_             = 0;
  num_free_functions_                   = 0;
  num_user_defined_types_               = 0;
  num_user_defined_instantiation_types_ = 0;
  function_                             = nullptr;
  use_any_node_                         = nullptr;

  if (!file_errors_array_.empty())
  {
    errors = GetErrorList();
    file_errors_array_.clear();
    return false;
  }
  errors.clear();
  return true;
}

void Analyser::EnforceLedgerRestrictions(BlockNodePtr const &block_node)
{
  LedgerRestrictionMetadata metadata;

  ValidateBlock(block_node, metadata);

  CheckInitFunctionUnique(metadata);
  if (CheckSynergeticFunctionsPresentAndUnique(metadata))
  {
    CheckSynergeticContract(metadata);
  }
}

void Analyser::ValidateBlock(BlockNodePtr const &block_node, LedgerRestrictionMetadata &metadata)
{
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      BlockNodePtr file_node = ConvertToBlockNodePtr(child);
      filename_              = file_node->text;
      ValidateBlock(file_node, metadata);
      break;
    }
    case NodeKind::ContractDefinition:
    {
      ExpressionNodePtr contract_name_node = ConvertToExpressionNodePtr(child->children[0]);
      if (contract_name_node->type)
      {
        ValidateBlock(ConvertToBlockNodePtr(child), metadata);
      }
      break;
    }
    case NodeKind::ContractFunction:
    {
      ExpressionNodePtr function_name_node = ConvertToExpressionNodePtr(child->children[1]);
      if (function_name_node->function)
      {
        ValidateFunctionAnnotations(child);
        ValidateFunctionPrototype(child, metadata);
      }
      break;
    }
    case NodeKind::FreeFunctionDefinition:
    {
      ExpressionNodePtr function_name_node = ConvertToExpressionNodePtr(child->children[1]);
      if (function_name_node->function)
      {
        ValidateFunctionAnnotations(ConvertToBlockNodePtr(child));
        ValidateFunctionPrototype(ConvertToBlockNodePtr(child), metadata);
      }
      break;
    }
    default:
    {
      break;
    }
    }  // switch
  }
}

void Analyser::ValidateFunctionAnnotations(NodePtr const &function_node)
{
  {
    auto const &function_name_node = ConvertToExpressionNodePtr(function_node->children[1]);
    FETCH_UNUSED(function_name_node);
    assert(function_name_node->function != nullptr);
  }

  auto const &annotations_node = function_node->children[0];
  if (annotations_node == nullptr)
  {
    return;
  }
  assert(annotations_node->node_kind == NodeKind::Annotations);

  if (annotations_node->children.size() != 1u)
  {
    AddError(annotations_node->line, "Functions are only allowed to bear one annotation");
  }

  for (auto const &annotation : annotations_node->children)
  {
    auto const &text = annotation->text;
    if (text != INIT_ANNOTATION && text != ACTION_ANNOTATION && text != QUERY_ANNOTATION &&
        text != PROBLEM_ANNOTATION && text != OBJECTIVE_ANNOTATION && text != WORK_ANNOTATION &&
        text != CLEAR_ANNOTATION)
    {
      AddError(annotation->line, "Invalid annotation '" + text + "'");
    }
  }
}

void Analyser::ValidateFunctionPrototype(NodePtr const &            function_node,
                                         LedgerRestrictionMetadata &metadata)
{
  auto const &function_name_node = ConvertToExpressionNodePtr(function_node->children[1]);
  assert(function_name_node->function != nullptr);

  auto const &annotations = function_node->children[0];

  bool is_action = false;
  bool is_init   = false;
  bool is_query  = false;

  assert(!annotations || annotations->node_kind == NodeKind::Annotations);

  switch (function_node->node_kind)
  {
  case NodeKind::ContractFunction:
  {
    if (HasAnnotation(annotations, ACTION_ANNOTATION))
    {
      is_action = true;
    }
    else
    {
      AddError(function_name_node->line, "Contract functions must carry the @action annotation");
    }
  }
  break;
  case NodeKind::FreeFunctionDefinition:
  {
    if (HasAnnotation(annotations, ACTION_ANNOTATION))
    {
      is_action = true;
    }
    if (HasAnnotation(annotations, INIT_ANNOTATION))
    {
      is_init = true;
      metadata.init_functions.emplace_back(function_node, filename_);
    }
    if (HasAnnotation(annotations, QUERY_ANNOTATION))
    {
      is_query = true;
    }
    if (HasAnnotation(annotations, CLEAR_ANNOTATION))
    {
      metadata.clear_functions.emplace_back(function_node, filename_);
    }
    if (HasAnnotation(annotations, OBJECTIVE_ANNOTATION))
    {
      metadata.objective_functions.emplace_back(function_node, filename_);
    }
    if (HasAnnotation(annotations, PROBLEM_ANNOTATION))
    {
      metadata.problem_functions.emplace_back(function_node, filename_);
    }
    if (HasAnnotation(annotations, WORK_ANNOTATION))
    {
      metadata.work_functions.emplace_back(function_node, filename_);
    }
  }
  break;
  default:
    assert(false);
    break;
  }

  auto const &return_type = function_name_node->function->return_type;

  if (is_action)
  {
    if (return_type != int64_type_ && return_type != void_type_)
    {
      AddError(function_name_node->line,
               "@action functions must either return Int64 or have no return type");
    }
  }

  if (is_init)
  {
    if (return_type != int64_type_ && return_type != void_type_)
    {
      AddError(function_name_node->line,
               "@init functions must either return Int64 or have no return type");
    }

    auto const param_types = function_name_node->function->parameter_types;
    if (!param_types.empty() && !(param_types.size() == 1 && param_types.back() == address_type_))
    {
      AddError(function_name_node->line,
               "@init functions must accept either no parameters or exactly one parameter of type "
               "Address");
    }
  }

  if (is_query)
  {
    if (return_type == void_type_)
    {
      AddError(function_name_node->line, "@query functions must have a return type");
    }
  }
}

void Analyser::CheckInitFunctionUnique(LedgerRestrictionMetadata const &metadata)
{
  if (metadata.init_functions.size() > 1)
  {
    for (auto const &x : metadata.init_functions)
    {
      AddError(x.filename, x.node->line,
               "Multiple @init functions found; only one @init function is allowed per contract");
    }
  }
}

bool Analyser::CheckSynergeticFunctionsPresentAndUnique(LedgerRestrictionMetadata const &metadata)
{
  bool const clear_missing     = metadata.clear_functions.empty();
  bool const objective_missing = metadata.objective_functions.empty();
  bool const problem_missing   = metadata.problem_functions.empty();
  bool const work_missing      = metadata.work_functions.empty();

  if (clear_missing && objective_missing && problem_missing && work_missing)
  {
    return false;
  }

  if (metadata.clear_functions.size() == 1 && metadata.objective_functions.size() == 1 &&
      metadata.problem_functions.size() == 1 && metadata.work_functions.size() == 1)
  {
    return true;
  }

  std::ostringstream ss;
  if (clear_missing)
  {
    ss << " " << CLEAR_ANNOTATION;
  }
  if (objective_missing)
  {
    ss << " " << OBJECTIVE_ANNOTATION;
  }
  if (problem_missing)
  {
    ss << " " << PROBLEM_ANNOTATION;
  }
  if (work_missing)
  {
    ss << " " << WORK_ANNOTATION;
  }
  std::string const missing_synergetic_names = ss.str();

  bool const any_missing = clear_missing || objective_missing || problem_missing || work_missing;

  auto report_missing_and_multiple_synergetics = [this, missing_synergetic_names, any_missing](
                                                     auto const &annotation_name,
                                                     auto const &filename_nodes) {
    for (auto const &x : filename_nodes)
    {
      if (filename_nodes.size() > 1u)
      {
        std::ostringstream ss;
        ss << "Multiple " << annotation_name << " functions found; only one " << annotation_name
           << " function is allowed per contract";
        AddError(x.filename, x.node->line, ss.str());
      }

      if (any_missing)
      {
        std::ostringstream ss;
        ss << annotation_name
           << " function found, but the following synergetic functions are missing:"
           << missing_synergetic_names;
        AddError(x.filename, x.node->line, ss.str());
      }
    }
  };

  report_missing_and_multiple_synergetics(CLEAR_ANNOTATION, metadata.clear_functions);
  report_missing_and_multiple_synergetics(OBJECTIVE_ANNOTATION, metadata.objective_functions);
  report_missing_and_multiple_synergetics(PROBLEM_ANNOTATION, metadata.problem_functions);
  report_missing_and_multiple_synergetics(WORK_ANNOTATION, metadata.work_functions);

  return false;
}

void Analyser::CheckSynergeticContract(LedgerRestrictionMetadata const &metadata)
{
  assert(metadata.clear_functions.size() == 1);
  assert(metadata.objective_functions.size() == 1);
  assert(metadata.problem_functions.size() == 1);
  assert(metadata.work_functions.size() == 1);

  auto const &clear_file     = metadata.clear_functions.back().filename;
  auto const &objective_file = metadata.objective_functions.back().filename;
  auto const &problem_file   = metadata.problem_functions.back().filename;
  auto const &work_file      = metadata.work_functions.back().filename;

  auto const &clear_node     = metadata.clear_functions.back().node;
  auto const &objective_node = metadata.objective_functions.back().node;
  auto const &problem_node   = metadata.problem_functions.back().node;
  auto const &work_node      = metadata.work_functions.back().node;

  auto const &clear_function = *ConvertToExpressionNodePtr(clear_node->children[1])->function;
  auto const &objective_function =
      *ConvertToExpressionNodePtr(objective_node->children[1])->function;
  auto const &problem_function = *ConvertToExpressionNodePtr(problem_node->children[1])->function;
  auto const &work_function    = *ConvertToExpressionNodePtr(work_node->children[1])->function;

  // @problem
  TypePtr problem_param_type;
  if (!problem_function.parameter_types.empty())
  {
    problem_param_type = problem_function.parameter_types[0];
  }

  bool const problem_params_correct = (problem_function.parameter_types.size() == 1) &&
                                      (problem_param_type->name == "Array<StructuredData>");
  if (!problem_params_correct)
  {
    AddError(
        problem_file, problem_node->line,
        "The @problem function should accept a single parameter of type Array<StructuredData>");
  }

  if (problem_function.return_type == void_type_)
  {
    AddError(problem_file, problem_node->line, "The @problem function must have a return type");
    return;
  }

  // @work
  TypePtr work_first_param_type;
  TypePtr work_second_param_type;
  if (!work_function.parameter_types.empty())
  {
    work_first_param_type = work_function.parameter_types[0];
  }
  if (work_function.parameter_types.size() > 1)
  {
    work_second_param_type = work_function.parameter_types[1];
  }

  bool const work_params_correct = (work_function.parameter_types.size() == 2) &&
                                   (work_first_param_type == problem_function.return_type) &&
                                   (work_second_param_type->name == "UInt256");
  if (!work_params_correct)
  {
    AddError(work_file, work_node->line,
             "The @work function should accept two parameters: the first should have the same "
             "type as that returned by the @problem function; the second should be UInt256");
  }

  if (work_function.return_type == void_type_)
  {
    AddError(work_file, work_node->line, "The @work function must have a return type");
    return;
  }

  // @objective
  if (objective_function.return_type != int64_type_)
  {
    AddError(objective_file, objective_node->line, "The @objective function must return Int64");
  }

  TypePtr objective_first_param_type;
  TypePtr objective_second_param_type;
  if (!objective_function.parameter_types.empty())
  {
    objective_first_param_type = objective_function.parameter_types[0];
  }
  if (objective_function.parameter_types.size() > 1)
  {
    objective_second_param_type = objective_function.parameter_types[1];
  }

  bool const objective_params_correct =
      (objective_function.parameter_types.size() == 2) &&
      (objective_first_param_type == problem_function.return_type) &&
      (objective_second_param_type == work_function.return_type);
  if (!objective_params_correct)
  {
    AddError(objective_file, objective_node->line,
             "The @objective function should accept two parameters: the first should have the same "
             "type as that returned by the @problem function; the second should have the same type "
             "as that returned by the @work function");
  }

  // @clear
  if (clear_function.return_type != void_type_)
  {
    AddError(clear_file, clear_node->line, "The @clear function must not have a return type");
  }

  TypePtr clear_first_param_type;
  TypePtr clear_second_param_type;
  if (!clear_function.parameter_types.empty())
  {
    clear_first_param_type = clear_function.parameter_types[0];
  }
  if (clear_function.parameter_types.size() > 1)
  {
    clear_second_param_type = clear_function.parameter_types[1];
  }

  bool const clear_params_correct = (clear_function.parameter_types.size() == 2) &&
                                    (clear_first_param_type == problem_function.return_type) &&
                                    (clear_second_param_type == work_function.return_type);
  if (!clear_params_correct)
  {
    AddError(clear_file, clear_node->line,
             "The @clear function should accept two parameters: the first should have the same "
             "type as that returned by the @problem function; the second should have the same type "
             "as that returned by the @work function");
  }
}

void Analyser::AddError(uint16_t line, std::string const &message)
{
  AddError(filename_, line, message);
}

void Analyser::AddError(std::string const &filename, uint16_t line, std::string const &message)
{
  std::ostringstream stream;
  stream << filename << ": line " << line << ": error: " << message;
  Error error{line, stream.str()};
  for (auto &file_errors_it : file_errors_array_)
  {
    if (filename == file_errors_it.filename)
    {
      file_errors_it.errors.push_back(std::move(error));
      return;
    }
  }
  FileErrors file_errors(filename);
  file_errors.errors.push_back(std::move(error));
  file_errors_array_.push_back(std::move(file_errors));
}

void Analyser::CheckLocals(uint16_t line)
{
  if (function_->num_locals++ >= MAX_LOCALS_PER_FUNCTION)
  {
    throw FatalErrorException(filename_, line, "maximum number of local variables exceeded");
  }
}

void Analyser::BuildBlock(BlockNodePtr const &block_node)
{
  if (blocks_.size() >= MAX_NESTED_BLOCKS)
  {
    throw FatalErrorException(filename_, block_node->line,
                              "maximum number of nested blocks exceeded");
  }
  blocks_.push_back(block_node);
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      BlockNodePtr file_node = ConvertToBlockNodePtr(child);
      file_node->symbols     = CreateSymbolTable();
      filename_              = file_node->text;
      BuildBlock(file_node);
      break;
    }
    case NodeKind::ContractDefinition:
    {
      BuildContractDefinition(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::StructDefinition:
    {
      BuildStructDefinition(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::FreeFunctionDefinition:
    {
      BuildFreeFunctionDefinition(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::IfStatement:
    {
      for (NodePtr const &c : child->children)
      {
        BlockNodePtr block = ConvertToBlockNodePtr(c);
        block->symbols     = CreateSymbolTable();
        BuildBlock(block);
      }
      break;
    }
    case NodeKind::MemberFunctionDefinition:
    case NodeKind::WhileStatement:
    case NodeKind::ForStatement:
    case NodeKind::If:
    case NodeKind::ElseIf:
    case NodeKind::Else:
    {
      BlockNodePtr block = ConvertToBlockNodePtr(child);
      block->symbols     = CreateSymbolTable();
      BuildBlock(block);
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

void Analyser::BuildContractDefinition(BlockNodePtr const &contract_definition_node)
{
  contract_definition_node->symbols = CreateSymbolTable();
  ExpressionNodePtr contract_name_node =
      ConvertToExpressionNodePtr(contract_definition_node->children[0]);
  std::string const &contract_name = contract_name_node->text;
  if (num_contract_definitions_++ >= MAX_CONTRACT_DEFINITIONS)
  {
    throw FatalErrorException(filename_, contract_name_node->line,
                              "maximum number of contract definitions exceeded");
  }
  // check symbols_ as well here?
  if (contract_definitions_.Find(contract_name) || root_->symbols->Find(contract_name))
  {
    AddError(contract_name_node->line, "symbol '" + contract_name + "' is already defined");
    return;
  }
  TypePtr contract_type  = CreateType(TypeKind::UserDefinedContract, contract_name);
  contract_type->symbols = CreateSymbolTable();
  contract_definitions_.Add(contract_name, contract_type);
  contract_name_node->type = contract_type;
  BuildBlock(contract_definition_node);
}

void Analyser::BuildStructDefinition(BlockNodePtr const &struct_definition_node)
{
  struct_definition_node->symbols = CreateSymbolTable();
  ExpressionNodePtr struct_name_node =
      ConvertToExpressionNodePtr(struct_definition_node->children[0]);
  std::string const &struct_name = struct_name_node->text;
  if (num_user_defined_types_++ >= MAX_USER_DEFINED_TYPES)
  {
    throw FatalErrorException(filename_, struct_name_node->line,
                              "maximum number of user defined types exceeded");
  }
  // check symbols_ as well here?
  if (contract_definitions_.Find(struct_name) || root_->symbols->Find(struct_name))
  {
    AddError(struct_name_node->line, "symbol '" + struct_name + "' is already defined");
    return;
  }
  TypePtr struct_type  = CreateType(TypeKind::UserDefinedStruct, struct_name);
  struct_type->symbols = CreateSymbolTable();
  root_->symbols->Add(struct_type);
  struct_name_node->type = struct_type;
  BuildBlock(struct_definition_node);
}

void Analyser::BuildFreeFunctionDefinition(BlockNodePtr const &function_definition_node)
{
  function_definition_node->symbols = CreateSymbolTable();
  ExpressionNodePtr function_name_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  std::string const &function_name = function_name_node->text;
  if (num_free_functions_++ >= MAX_FREE_FUNCTIONS)
  {
    throw FatalErrorException(filename_, function_name_node->line,
                              "maximum number of functions exceeded");
  }
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
  }
  else
  {
    function_group = CreateFunctionGroup(function_name, nullptr);
    root_->symbols->Add(function_group);
  }
  function_name_node->function_group = function_group;
  BuildBlock(function_definition_node);
}

void Analyser::PreAnnotateBlock(BlockNodePtr const &block_node)
{
  blocks_.push_back(block_node);
  for (NodePtr const &child : block_node->block_children)
  {
    switch (child->node_kind)
    {
    case NodeKind::File:
    {
      BlockNodePtr file_node = ConvertToBlockNodePtr(child);
      filename_              = file_node->text;
      PreAnnotateBlock(file_node);
      break;
    }
    case NodeKind::PersistentStatement:
    {
      PreAnnotatePersistentStatement(child);
      break;
    }
    case NodeKind::ContractDefinition:
    {
      PreAnnotateContractDefinition(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::ContractFunction:
    {
      PreAnnotateContractFunction(block_node, child);
      break;
    }
    case NodeKind::StructDefinition:
    {
      PreAnnotateStructDefinition(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::MemberFunctionDefinition:
    {
      PreAnnotateMemberFunctionDefinition(block_node, ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::MemberVarDeclarationStatement:
    {
      PreAnnotateMemberVarDeclarationStatement(block_node, child);
      break;
    }
    case NodeKind::FreeFunctionDefinition:
    {
      PreAnnotateFreeFunctionDefinition(ConvertToBlockNodePtr(child));
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

void Analyser::PreAnnotatePersistentStatement(NodePtr const &persistent_statement_node)
{
  ExpressionNodePtr state_name_node =
      ConvertToExpressionNodePtr(persistent_statement_node->children[0]);
  ExpressionNodePtr modifier_node =
      ConvertToExpressionNodePtr(persistent_statement_node->children[1]);
  ExpressionNodePtr  type_node = ConvertToExpressionNodePtr(persistent_statement_node->children[2]);
  std::string const &state_name = state_name_node->text;
  if (num_state_definitions_++ >= MAX_STATE_DEFINITIONS)
  {
    throw FatalErrorException(filename_, state_name_node->line,
                              "maximum number of state definitions exceeded");
  }
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
    if (num_user_defined_instantiation_types_++ >= MAX_USER_DEFINED_INSTANTIATION_TYPES)
    {
      throw FatalErrorException(filename_, state_name_node->line,
                                "maximum number of user defined instantiation types exceeded");
    }
    instantation_type = InternalCreateTemplateInstantiationType(
        TypeKind::UserDefinedTemplateInstantiation, template_type, {managed_type});
    root_->symbols->Add(instantation_type);
  }
  state_name_node->type = instantation_type;
  type_node->type       = managed_type;
  state_definitions_.Add(state_name, instantation_type);
}

void Analyser::PreAnnotateContractDefinition(BlockNodePtr const &contract_definition_node)
{
  ExpressionNodePtr contract_name_node =
      ConvertToExpressionNodePtr(contract_definition_node->children[0]);
  TypePtr contract_type = contract_name_node->type;
  if (!contract_type)
  {
    return;
  }
  PreAnnotateBlock(contract_definition_node);
}

void Analyser::PreAnnotateContractFunction(BlockNodePtr const &contract_definition_node,
                                           NodePtr const &     function_node)
{
  ExpressionNodePtr contract_name_node =
      ConvertToExpressionNodePtr(contract_definition_node->children[0]);
  TypePtr contract_type = contract_name_node->type;
  if (!contract_type)
  {
    return;
  }
  ExpressionNodePtr  function_name_node = ConvertToExpressionNodePtr(function_node->children[1]);
  std::string const &function_name      = function_name_node->text;
  if (contract_type->num_functions++ >= MAX_FUNCTIONS_PER_CONTRACT)
  {
    throw FatalErrorException(filename_, function_name_node->line,
                              "maximum number of contract functions exceeded");
  }
  FunctionGroupPtr function_group;
  SymbolPtr        symbol = contract_definition_node->symbols->Find(function_name);
  if (symbol)
  {
    if (!symbol->IsFunctionGroup())
    {
      AddError(function_name_node->line, "symbol '" + function_name + "' is already defined");
      return;
    }
    function_group = ConvertToFunctionGroupPtr(symbol);
  }
  else
  {
    function_group = CreateFunctionGroup(function_name, nullptr);
    contract_definition_node->symbols->Add(function_group);
    contract_type->symbols->Add(function_group);
    function_name_node->function_group = function_group;
  }
  ExpressionNodePtrArray parameter_nodes;
  TypePtrArray           parameter_types;
  VariablePtrArray       parameter_variables;
  TypePtr                return_type;
  if (!PreAnnotatePrototype(function_node, parameter_nodes, parameter_types, parameter_variables,
                            return_type))
  {
    return;
  }
  if (FindFunction(contract_type, function_group, parameter_nodes))
  {
    AddError(function_name_node->line,
             "function '" + function_name + "' is already defined with the same parameter types");
    return;
  }
  FunctionPtr function = CreateUserDefinedContractFunction(
      contract_type, function_name, parameter_types, parameter_variables, return_type);
  function_group->functions.push_back(function);
  function_name_node->function = function;
}

void Analyser::PreAnnotateStructDefinition(BlockNodePtr const &struct_definition_node)
{
  ExpressionNodePtr struct_name_node =
      ConvertToExpressionNodePtr(struct_definition_node->children[0]);
  TypePtr struct_type = struct_name_node->type;
  if (!struct_type)
  {
    return;
  }
  PreAnnotateBlock(struct_definition_node);
  // If a default constructor wasn't explicitly defined, provide one
  FunctionGroupPtr function_group;
  FunctionPtr      function;
  SymbolPtr        symbol = struct_definition_node->symbols->Find(CONSTRUCTOR);
  if (symbol)
  {
    function_group = ConvertToFunctionGroupPtr(symbol);
    function       = FindFunction(struct_type, function_group, {});
    if (!function)
    {
      function = CreateUserDefinedConstructor(struct_type, {}, {});
      function_group->functions.push_back(function);
    }
  }
  else
  {
    function_group = CreateFunctionGroup(CONSTRUCTOR, struct_type);
    function       = CreateUserDefinedConstructor(struct_type, {}, {});
    function_group->functions.push_back(function);
    struct_definition_node->symbols->Add(function_group);
    struct_type->symbols->Add(function_group);
  }
  // Store information on the struct's default constructor
  struct_name_node->function_group = function_group;
  struct_name_node->function       = function;
}

void Analyser::PreAnnotateMemberFunctionDefinition(BlockNodePtr const &struct_definition_node,
                                                   BlockNodePtr const &function_definition_node)
{
  ExpressionNodePtr struct_name_node =
      ConvertToExpressionNodePtr(struct_definition_node->children[0]);
  TypePtr struct_type = struct_name_node->type;
  if (!struct_type)
  {
    return;
  }
  ExpressionNodePtr function_name_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  std::string function_name = function_name_node->text;
  if (struct_type->num_functions++ >= MAX_MEMBER_FUNCTIONS_PER_TYPE)
  {
    throw FatalErrorException(filename_, function_name_node->line,
                              "maximum number of member functions exceeded");
  }
  FunctionGroupPtr function_group;
  std::string      symbol_name;
  bool             is_constructor = function_name == struct_type->name;
  if (is_constructor)
  {
    symbol_name = CONSTRUCTOR;
  }
  else
  {
    symbol_name = function_name;
  }
  SymbolPtr symbol = struct_definition_node->symbols->Find(symbol_name);
  if (symbol)
  {
    if (!symbol->IsFunctionGroup())
    {
      AddError(function_name_node->line, "symbol '" + function_name + "' is already defined");
      return;
    }
    function_group = ConvertToFunctionGroupPtr(symbol);
  }
  else
  {
    if (is_constructor)
    {
      std::size_t       index = function_definition_node->children.size() - 1;
      ExpressionNodePtr return_type_node =
          ConvertToExpressionNodePtr(function_definition_node->children[index]);
      if (return_type_node)
      {
        AddError(function_name_node->line, "constructor '" + function_name + "' has return type");
        return;
      }
    }
    function_group = CreateFunctionGroup(symbol_name, struct_type);
    struct_definition_node->symbols->Add(function_group);
    struct_type->symbols->Add(function_group);
    function_name_node->function_group = function_group;
  }
  ExpressionNodePtrArray parameter_nodes;
  TypePtrArray           parameter_types;
  VariablePtrArray       parameter_variables;
  TypePtr                return_type;
  if (!PreAnnotatePrototype(function_definition_node, parameter_nodes, parameter_types,
                            parameter_variables, return_type))
  {
    return;
  }
  if (FindFunction(struct_type, function_group, parameter_nodes))
  {
    std::string text = is_constructor ? "constructor" : "function";
    AddError(function_name_node->line,
             text + " '" + function_name + "' is already defined with the same parameter types");
    return;
  }
  for (auto const &parameter_variable : parameter_variables)
  {
    function_definition_node->symbols->Add(parameter_variable);
  }
  FunctionPtr function;
  if (is_constructor)
  {
    function = CreateUserDefinedConstructor(struct_type, parameter_types, parameter_variables);
  }
  else
  {
    function = CreateUserDefinedMemberFunction(struct_type, function_name, parameter_types,
                                               parameter_variables, return_type);
  }
  function_group->functions.push_back(function);
  function_name_node->function = function;
}

void Analyser::PreAnnotateMemberVarDeclarationStatement(BlockNodePtr const &struct_definition_node,
                                                        NodePtr const &     var_statement_node)
{
  ExpressionNodePtr struct_name_node =
      ConvertToExpressionNodePtr(struct_definition_node->children[0]);
  TypePtr struct_type = struct_name_node->type;
  if (!struct_type)
  {
    return;
  }
  ExpressionNodePtr variable_name_node =
      ConvertToExpressionNodePtr(var_statement_node->children[0]);
  std::string const &variable_name = variable_name_node->text;
  if (struct_type->num_variables++ >= MAX_MEMBER_VARIABLES_PER_TYPE)
  {
    throw FatalErrorException(filename_, variable_name_node->line,
                              "maximum number of member variables exceeded");
  }
  SymbolPtr symbol = struct_definition_node->symbols->Find(variable_name);
  if (symbol)
  {
    AddError(variable_name_node->line, "symbol '" + variable_name + "' is already defined");
    return;
  }
  ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
  if (!AnnotateTypeExpression(type_node))
  {
    return;
  }
  if (type_node->type->IsUserDefinedStruct())
  {
    AddError(variable_name_node->line, "member variable '" + variable_name +
                                           "' of user defined type '" + type_node->type->name +
                                           "' is not supported");
    return;
  }
  VariablePtr variable =
      CreateVariable(VariableKind::Member, variable_name, type_node->type, struct_type);
  struct_definition_node->symbols->Add(variable);
  struct_type->symbols->Add(variable);
  variable_name_node->variable = variable;
}

void Analyser::PreAnnotateFreeFunctionDefinition(BlockNodePtr const &function_definition_node)
{
  ExpressionNodePtr function_name_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  std::string const &function_name  = function_name_node->text;
  FunctionGroupPtr   function_group = function_name_node->function_group;
  if (!function_group)
  {
    return;
  }
  ExpressionNodePtrArray parameter_nodes;
  TypePtrArray           parameter_types;
  VariablePtrArray       parameter_variables;
  TypePtr                return_type;
  if (!PreAnnotatePrototype(function_definition_node, parameter_nodes, parameter_types,
                            parameter_variables, return_type))
  {
    return;
  }
  if (FindFunction(nullptr, function_group, parameter_nodes))
  {
    AddError(function_name_node->line,
             "function '" + function_name + "' is already defined with the same parameter types");
    return;
  }
  for (auto const &parameter_variable : parameter_variables)
  {
    function_definition_node->symbols->Add(parameter_variable);
  }
  FunctionPtr function = CreateUserDefinedFreeFunction(function_name, parameter_types,
                                                       parameter_variables, return_type);
  function_group->functions.push_back(function);
  function_name_node->function = function;
}

bool Analyser::PreAnnotatePrototype(NodePtr const &         prototype_node,
                                    ExpressionNodePtrArray &parameter_nodes,
                                    TypePtrArray &          parameter_types,
                                    VariablePtrArray &parameter_variables, TypePtr &return_type)
{
  parameter_nodes.clear();
  parameter_types.clear();
  parameter_variables.clear();
  StringSet parameter_names;

  auto const count          = prototype_node->children.size();
  auto const num_parameters = (count - 3) / 2;

  bool ok = true;
  for (std::size_t i = 0; i < num_parameters; ++i)
  {
    ExpressionNodePtr parameter_node =
        ConvertToExpressionNodePtr(prototype_node->children[2 + i * 2]);
    std::string const &parameter_name = parameter_node->text;
    if (parameter_names.Find(parameter_name))
    {
      AddError(parameter_node->line, "parameter name '" + parameter_name + "' is already defined");
      ok = false;
      continue;
    }
    parameter_names.Add(parameter_name);
    ExpressionNodePtr parameter_type_node =
        ConvertToExpressionNodePtr(prototype_node->children[3 + i * 2]);
    TypePtr parameter_type = FindType(parameter_type_node);
    if (!parameter_type)
    {
      AddError(parameter_type_node->line, "unknown type '" + parameter_type_node->text + "'");
      ok = false;
      continue;
    }
    if (parameter_nodes.size() == MAX_PARAMETERS_PER_FUNCTION)
    {
      AddError(parameter_node->line, "maximum number of function parameters exceeded");
      return false;
    }
    parameter_type_node->type = parameter_type;
    VariablePtr parameter_variable =
        CreateVariable(VariableKind::Parameter, parameter_name, parameter_type, nullptr);
    parameter_node->variable = parameter_variable;
    parameter_node->type     = parameter_variable->type;
    parameter_types.push_back(parameter_type);
    parameter_variables.push_back(std::move(parameter_variable));
    parameter_nodes.push_back(std::move(parameter_node));
  }

  ExpressionNodePtr return_type_node =
      ConvertToExpressionNodePtr(prototype_node->children[std::size_t(count - 1)]);
  if (return_type_node)
  {
    return_type = FindType(return_type_node);
    if (!return_type)
    {
      AddError(return_type_node->line, "unknown type '" + return_type_node->text + "'");
      ok = false;
    }
    return_type_node->type = return_type;
  }
  else
  {
    return_type = void_type_;
  }

  return ok;
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
      BlockNodePtr file_node = ConvertToBlockNodePtr(child);
      filename_              = file_node->text;
      AnnotateBlock(file_node);
      break;
    }
    case NodeKind::PersistentStatement:
    case NodeKind::ContractDefinition:
    case NodeKind::ContractFunction:
    {
      // nothing to do
      break;
    }
    case NodeKind::StructDefinition:
    {
      AnnotateStructDefinition(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::MemberFunctionDefinition:
    case NodeKind::FreeFunctionDefinition:
    {
      AnnotateFunctionDefinition(ConvertToBlockNodePtr(child));
      break;
    }
    case NodeKind::MemberVarDeclarationStatement:
    {
      // nothing to do
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
    case NodeKind::LocalVarDeclarationStatement:
    case NodeKind::LocalVarDeclarationTypedAssignmentStatement:
    case NodeKind::LocalVarDeclarationTypelessAssignmentStatement:
    {
      AnnotateLocalVarStatement(block_node, child);
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
    case NodeKind::Null:
    case NodeKind::InitialiserList:
    {
      AddError(child->line, "unable to infer type");
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
        use_any_node_->children.push_back(std::move(child));
      }
    }
  }

  if (is_loop)
  {
    loops_.pop_back();
  }
  blocks_.pop_back();
}

void Analyser::AnnotateStructDefinition(BlockNodePtr const &struct_definition_node)
{
  ExpressionNodePtr struct_name_node =
      ConvertToExpressionNodePtr(struct_definition_node->children[0]);
  TypePtr struct_type = struct_name_node->type;
  if (!struct_type)
  {
    return;
  }
  AnnotateBlock(struct_definition_node);
}

void Analyser::AnnotateFunctionDefinition(BlockNodePtr const &function_definition_node)
{
  ExpressionNodePtr function_name_node =
      ConvertToExpressionNodePtr(function_definition_node->children[1]);
  if (!function_name_node->function)
  {
    return;
  }
  function_     = function_name_node->function;
  use_any_node_ = nullptr;
  AnnotateBlock(function_definition_node);
  bool is_constructor = function_->function_kind == FunctionKind::UserDefinedConstructor;
  if (file_errors_array_.empty() && !is_constructor && !function_->return_type->IsVoid())
  {
    if (TestBlock(function_definition_node))
    {
      AddError(function_definition_node->block_terminator_line,
               "control reaches end of function without returning a value");
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
  CheckLocals(name_node->line);
  // Note: variable is created with no type to mark as initially unresolved
  VariablePtr variable = CreateVariable(VariableKind::For, name, nullptr, nullptr);
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
    if (!ConvertNode(child_node, any_integer_type_))
    {
      ++problems;
      AddError(child_node->line, "integral type expected");
      continue;
    }
    nodes.push_back(std::move(child_node));
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
      // Note that the "null" literal will not be accepted here
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
  CheckLocals(variable_name_node->line);
  VariablePtr variable = CreateVariable(VariableKind::Use, variable_name, type, nullptr);
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
    CheckLocals(use_any_statement_node->line);
    VariablePtr variable = CreateVariable(VariableKind::UseAny, name, type, nullptr);
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
  if (!ConvertNode(initialiser_node, string_type_))
  {
    AddError(initialiser_node->line, "contract initialiser must be String type");
    return;
  }
  CheckLocals(contract_variable_node->line);
  VariablePtr variable =
      CreateVariable(VariableKind::Local, contract_variable_name, contract_type, nullptr);
  parent_block_node->symbols->Add(variable);
  contract_variable_node->variable = variable;
  contract_type_node->type         = contract_type;
}

void Analyser::AnnotateLocalVarStatement(BlockNodePtr const &parent_block_node,
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
  CheckLocals(name_node->line);
  // Note: variable is created with no type to mark as initially unresolved
  VariablePtr variable = CreateVariable(VariableKind::Local, name, nullptr, nullptr);
  parent_block_node->symbols->Add(variable);
  name_node->variable = variable;

  if (var_statement_node->node_kind == NodeKind::LocalVarDeclarationStatement)
  {
    ExpressionNodePtr type_node = ConvertToExpressionNodePtr(var_statement_node->children[1]);
    if (!AnnotateTypeExpression(type_node))
    {
      return;
    }
    variable->type = type_node->type;
  }
  else if (var_statement_node->node_kind == NodeKind::LocalVarDeclarationTypedAssignmentStatement)
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
    if (!ConvertNode(expression_node, type_node->type))
    {
      AddError(type_node->line, "incompatible types");
      return;
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
    if (!expression_node->IsConcrete())
    {
      AddError(expression_node->line, "unable to infer type");
      return;
    }
    variable->type = expression_node->type;
  }
}

void Analyser::AnnotateReturnStatement(NodePtr const &return_statement_node)
{
  bool is_constructor = function_->function_kind == FunctionKind::UserDefinedConstructor;
  if (return_statement_node->children.size() == 1)
  {
    if (is_constructor || function_->return_type->IsVoid())
    {
      AddError(return_statement_node->line, "return supplies a value");
      return;
    }
    ExpressionNodePtr expression_node =
        ConvertToExpressionNodePtr(return_statement_node->children[0]);
    if (!AnnotateExpression(expression_node))
    {
      return;
    }
    if (!ConvertNode(expression_node, function_->return_type))
    {
      AddError(expression_node->line, "incompatible types");
      return;
    }
  }
  else
  {
    if (!is_constructor && !function_->return_type->IsVoid())
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
    if (!ConvertNode(expression_node, bool_type_))
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
  if (!ConvertNode(rhs, lhs->type))
  {
    AddError(node->line, "incompatible types");
    return false;
  }
  assert(lhs->type == rhs->type);
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
  if (!ConvertNode(lhs, any_integer_type_) || !ConvertNode(rhs, any_integer_type_))
  {
    AddError(node->line, "matching integral operands expected");
    return false;
  }
  if (lhs->type != rhs->type)
  {
    AddError(node->line, "matching integral operands expected");
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

  std::size_t const      num_indexes = lhs->children.size() - 1;
  ExpressionNodePtrArray index_nodes;
  for (std::size_t i = 1; i <= num_indexes; ++i)
  {
    ExpressionNodePtr index_node = ConvertToExpressionNodePtr(lhs->children[i]);
    index_nodes.push_back(std::move(index_node));
  }

  SymbolPtr setter_symbol = symbols->Find(SET_INDEXED_VALUE);
  if (!setter_symbol)
  {
    AddError(container_node->line, "unable to find matching index operator for type '" +
                                       container_node->type->name + "'");
    return false;
  }

  FunctionGroupPtr setter_fg = ConvertToFunctionGroupPtr(setter_symbol);

  index_nodes.push_back(lhs);
  FunctionPtr setter_f = FindFunction(container_node->type, setter_fg, index_nodes);
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
    AddError(node->line, "invalid use of type '" + node->type->name + "'");
    return false;
  }
  if (node->IsFunctionGroupExpression())
  {
    AddError(node->line, "invalid use of function '" + node->function_group->name + "'");
    return false;
  }
  // node->IsVariableExpression()
  // node->IsLVExpression()
  // node->IsRVExpression()
  if (node->IsVariableExpression() && node->type->IsUserDefinedContract())
  {
    AddError(node->line, "invalid use of contract variable '" + node->variable->name + "'");
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
      // A plain identifier may also be a member function of a user defined type
      SetFunctionGroupExpression(node, function_group, function_group->user_defined_type);
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
      // A plain identifier may also be a member variable of a user defined type
      SetVariableExpression(node, variable, variable->user_defined_type);
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
  case NodeKind::Fixed128:
  {
    SetRVExpression(node, fixed128_type_);
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
  ExpressionNodePtr lhs             = ConvertToExpressionNodePtr(node->children[0]);
  ExpressionNodePtr rhs             = ConvertToExpressionNodePtr(node->children[1]);
  bool              lhs_is_concrete = lhs->IsConcrete();
  bool              rhs_is_concrete = rhs->IsConcrete();
  if (!lhs_is_concrete && !rhs_is_concrete)
  {
    AddError(node->line, "unable to infer types");
    return false;
  }
  if (lhs_is_concrete)
  {
    if (!ConvertNode(rhs, lhs->type))
    {
      AddError(node->line, "incompatible types");
      return false;
    }
  }
  else  // rhs_is_concrete
  {
    if (!ConvertNode(lhs, rhs->type))
    {
      AddError(node->line, "incompatible types");
      return false;
    }
  }
  assert(lhs->type == rhs->type);
  if (!IsOperatorEnabled(lhs->type, op))
  {
    AddError(node->line, "operator not supported");
    return false;
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
  if (lhs->IsNull() || rhs->IsNull())
  {
    AddError(node->line, "incompatible types");
    return false;
  }
  bool lhs_is_concrete = lhs->IsConcrete();
  bool rhs_is_concrete = rhs->IsConcrete();
  if (!lhs_is_concrete && !rhs_is_concrete)
  {
    AddError(node->line, "unable to infer types");
    return false;
  }
  if (lhs_is_concrete)
  {
    if (!ConvertNode(rhs, lhs->type))
    {
      AddError(node->line, "incompatible types");
      return false;
    }
  }
  else  // rhs_is_concrete
  {
    if (!ConvertNode(lhs, rhs->type))
    {
      AddError(node->line, "incompatible types");
      return false;
    }
  }
  assert(lhs->type == rhs->type);
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
  if (!ConvertNode(lhs, bool_type_) || !ConvertNode(rhs, bool_type_))
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
  if (!ConvertNode(operand, bool_type_))
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
  if (!ConvertNode(operand, any_integer_type_))
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
  if (!operand->IsConcrete())
  {
    AddError(node->line, "unable to infer type");
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
  if (!ConvertNode(lhs, any_integer_type_) || !ConvertNode(rhs, any_integer_type_))
  {
    AddError(node->line, "matching integral operands expected");
    return false;
  }
  if (lhs->type != rhs->type)
  {
    AddError(node->line, "matching integral operands expected");
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
  if (!lhs->IsConcrete())
  {
    // Prevent null[i, j] and {3, 4, 5}[i, j] and contractvariable[i, j]
    AddError(lhs->line, "operand does not support index operator");
    return false;
  }
  if (lhs->type->IsPrimitive())
  {
    AddError(lhs->line, "type '" + lhs->type->name + "' does not support index operator");
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
  std::size_t const      num_indexes = node->children.size() - 1;
  ExpressionNodePtrArray index_nodes;
  for (std::size_t i = 1; i <= num_indexes; ++i)
  {
    ExpressionNodePtr index_node = ConvertToExpressionNodePtr(node->children[i]);
    if (!AnnotateExpression(index_node))
    {
      return false;
    }
    index_nodes.push_back(std::move(index_node));
  }
  SymbolPtr symbol = symbols->Find(GET_INDEXED_VALUE);
  if (!symbol)
  {
    AddError(lhs->line,
             "unable to find matching index operator for type '" + lhs->type->name + "'");
    return false;
  }
  FunctionGroupPtr fg = ConvertToFunctionGroupPtr(symbol);
  FunctionPtr      f  = FindFunction(lhs->type, fg, index_nodes);
  if (!f)
  {
    AddError(lhs->line,
             "unable to find matching index operator for type '" + lhs->type->name + "'");
    return false;
  }
  TypePtr return_type = ResolveReturnType(f->return_type, lhs->type);
  SetLVExpression(node, return_type);
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
    AddError(lhs->line, "type '" + lhs->type->name + "' does not support member-access operator");
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
    SetFunctionGroupExpression(node, function_group, lhs->type);
    return true;
  }
  if (member_symbol->IsType())
  {
    // member is a type name
    AddError(node->line, "not supported");
    return false;
  }
  // member is a variable name
  if (lhs_is_type_expression)
  {
    // static member variable
    AddError(lhs->line,
             "'" + lhs->type->name + "' has no static member named '" + member_name + "'");
    return false;
  }
  // member variable
  VariablePtr variable = ConvertToVariablePtr(member_symbol);
  SetVariableExpression(node, variable, lhs->type);
  return true;
}

bool Analyser::AnnotateInvokeOp(ExpressionNodePtr const &node)
{
  ExpressionNodePtr lhs = ConvertToExpressionNodePtr(node->children[0]);
  if (!InternalAnnotateExpression(lhs))
  {
    return false;
  }
  ExpressionNodePtrArray parameter_nodes;
  for (std::size_t i = 1; i < node->children.size(); ++i)
  {
    ExpressionNodePtr parameter_node = ConvertToExpressionNodePtr(node->children[i]);
    if (!AnnotateExpression(parameter_node))
    {
      return false;
    }
    parameter_nodes.push_back(std::move(parameter_node));
  }
  if (lhs->IsFunctionGroupExpression())
  {
    // Note that lhs->owner is null for free functions
    FunctionPtr f = FindFunction(lhs->owner, lhs->function_group, parameter_nodes);
    if (!f)
    {
      // No matching function, or ambiguous
      AddError(lhs->line,
               "unable to find matching function for '" + lhs->function_group->name + "'");
      return false;
    }
    TypePtr return_type = ResolveReturnType(f->return_type, lhs->owner);
    SetRVExpression(node, return_type);
    node->function = f;
    return true;
  }
  if (lhs->IsTypeExpression())
  {
    // Type constructor
    if (lhs->type->IsPrimitive())
    {
      AddError(lhs->line, "type '" + lhs->type->name + "' is not constructible");
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
    FunctionPtr      f  = FindFunction(lhs->type, fg, parameter_nodes);
    if (!f)
    {
      // No matching constructor, or ambiguous
      AddError(lhs->line, "unable to find matching constructor for type '" + lhs->type->name + "'");
      return false;
    }
    SetRVExpression(node, lhs->type);
    node->function = f;
    return true;
  }
  // lhs->IsVariableExpression()
  // lhs->IsLVExpression()
  // lhs->IsRVExpression()
  // e.g.
  // null()
  // {3, 4, 5}()
  // contractvariable()
  // variable()
  // (a + b)()
  // array[index]()
  AddError(lhs->line, "operand does not support function-call operator");
  return false;
}

bool Analyser::AnnotateInitialiserList(ExpressionNodePtr const &node)
{
  node->type = initialiser_list_type_;
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
  node->type = initialiser_list_type_;
  return type->IsInstantiation() && (type->template_type == array_type_) &&
         ConvertInitialiserListToArray(node, type);
}

bool Analyser::ConvertInitialiserListToArray(ExpressionNodePtr const &node, TypePtr const &type)
{
  TypePtr const &element_type = type->types[0];
  for (NodePtr const &c : node->children)
  {
    auto child = ConvertToExpressionNodePtr(c);
    if (!ConvertNode(child, element_type))
    {
      return false;
    }
  }
  // All children were convertible to the element type, so set the type
  node->type = type;
  return true;
}

// Returns true if control is able to reach the end of the block
bool Analyser::TestBlock(BlockNodePtr const &block_node) const
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
  if (lhs->IsNull() || rhs->IsNull())
  {
    AddError(node->line, "incompatible types");
    return false;
  }
  bool lhs_is_concrete = lhs->IsConcrete();
  bool rhs_is_concrete = rhs->IsConcrete();
  if (!lhs_is_concrete && !rhs_is_concrete)
  {
    AddError(node->line, "unable to infer types");
    return false;
  }
  if (lhs_is_concrete)
  {
    if (IsOperatorEnabled(lhs->type, op))
    {
      if (ConvertNode(rhs, lhs->type))
      {
        SetRVExpression(node, lhs->type);
        return true;
      }
    }
    bool lhs_is_instantiation = lhs->type->IsInstantiation();
    if (lhs_is_instantiation && IsRightOperatorEnabled(lhs->type, op))
    {
      TypePtr const &lhs_inner_type = lhs->type->types[0];
      if (ConvertNode(rhs, lhs_inner_type))
      {
        SetRVExpression(node, lhs->type);
        return true;
      }
    }
  }
  if (rhs_is_concrete)
  {
    if (IsOperatorEnabled(rhs->type, op))
    {
      if (ConvertNode(lhs, rhs->type))
      {
        SetRVExpression(node, rhs->type);
        return true;
      }
    }
    bool rhs_is_instantiation = rhs->type->IsInstantiation();
    if (rhs_is_instantiation && IsLeftOperatorEnabled(rhs->type, op))
    {
      TypePtr const &rhs_inner_type = rhs->type->types[0];
      if (ConvertNode(lhs, rhs_inner_type))
      {
        SetRVExpression(node, rhs->type);
        return true;
      }
    }
  }
  AddError(node->line, "incompatible types");
  return false;
}

FunctionPtr Analyser::FindFunction(TypePtr const &type, FunctionGroupPtr const &function_group,
                                   ExpressionNodePtrArray const &parameter_nodes)
{
  FunctionPtrArray          functions;
  std::vector<TypePtrArray> array;
  for (FunctionPtr const &function : function_group->functions)
  {
    std::size_t const num_types = function->parameter_types.size();
    if (parameter_nodes.size() != num_types)
    {
      continue;
    }
    bool         match = true;
    TypePtrArray temp_resolved_types;
    for (std::size_t i = 0; i < num_types; ++i)
    {
      auto const &   parameter_node = parameter_nodes[i];
      TypePtr const &expected_type  = function->parameter_types[i];
      TypePtr        resolved_type  = ConvertNode(parameter_node, expected_type, type);
      if (!resolved_type)
      {
        match = false;
        break;
      }
      temp_resolved_types.push_back(std::move(resolved_type));
    }
    if (match)
    {
      functions.push_back(function);
      array.push_back(std::move(temp_resolved_types));
    }
  }
  if (functions.size() == 1)
  {
    TypePtrArray const &resolved_types = array[0];
    for (std::size_t i = 0; i < parameter_nodes.size(); ++i)
    {
      auto const &parameter_node = parameter_nodes[i];
      if (parameter_node->IsNull() || parameter_node->IsInitialiserList())
      {
        ConvertNode(parameter_node, resolved_types[i]);
      }
    }
    return functions[0];
  }
  // Matching function not found, or ambiguous
  return nullptr;
}

TypePtr Analyser::ConvertNode(ExpressionNodePtr const &node, TypePtr const &expected_type)
{
  return ConvertNode(node, expected_type, nullptr);
}

TypePtr Analyser::ConvertNode(ExpressionNodePtr const &node, TypePtr const &expected_type,
                              TypePtr const &type)
{
  TypePtr const &input_type = node->type;

  if (input_type->IsTemplate() || input_type->IsMeta() || input_type->IsGroup() ||
      input_type->IsUserDefinedContract())
  {
    return nullptr;
  }

  if ((expected_type == null_type_) || (expected_type == initialiser_list_type_) ||
      (expected_type == void_type_) || expected_type->IsUserDefinedContract())
  {
    return nullptr;
  }

  TypePtr comparison_type;
  if (expected_type->IsTemplate())
  {
    if (type && type->IsInstantiation())
    {
      comparison_type = type;
    }
    else
    {
      return nullptr;
    }
  }
  else if (expected_type == template_parameter1_type_)
  {
    if (type && type->IsInstantiation() && !type->types.empty())
    {
      comparison_type = type->types[0];
    }
    else
    {
      return nullptr;
    }
  }
  else if (expected_type == template_parameter2_type_)
  {
    if (type && type->IsInstantiation() && (type->types.size() >= 2))
    {
      comparison_type = type->types[1];
    }
    else
    {
      return nullptr;
    }
  }
  else
  {
    comparison_type = expected_type;
  }

  if (node->IsNull())
  {
    // Can only convert null literal to a known reference type
    if (comparison_type->IsClass() || comparison_type->IsInstantiation() ||
        comparison_type->IsUserDefinedStruct())
    {
      node->type = comparison_type;
      return comparison_type;
    }
    return nullptr;
  }

  if (node->IsInitialiserList())
  {
    // Can only convert initialiser list to a known reference type
    if (comparison_type->IsClass() || comparison_type->IsInstantiation() ||
        comparison_type->IsUserDefinedStruct())
    {
      if (ConvertInitialiserList(node, comparison_type))
      {
        node->type = comparison_type;
        return comparison_type;
      }
    }
    return nullptr;
  }

  if (input_type == void_type_)
  {
    // Can't convert Void to anything
    return nullptr;
  }

  if (comparison_type == any_type_)
  {
    return input_type;
  }

  if (comparison_type == any_primitive_type_)
  {
    return input_type->IsPrimitive() ? input_type : nullptr;
  }

  if (comparison_type->IsGroup())
  {
    for (auto const &possible_type : comparison_type->types)
    {
      if (input_type == possible_type)
      {
        return input_type;
      }
    }
    return nullptr;
  }

  return (input_type == comparison_type) ? input_type : nullptr;
}

TypePtr Analyser::ResolveReturnType(TypePtr const &return_type, TypePtr const &type)
{
  if (return_type->IsTemplate())
  {
    return type;
  }
  if (return_type == template_parameter1_type_)
  {
    return type->types[0];
  }
  if (return_type == template_parameter2_type_)
  {
    return type->types[1];
  }
  return return_type;
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
    if (!symbol->IsType())
    {
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
      parameter_type_node->type              = parameter_type;
      TypePtr const &expected_parameter_type = template_type->types[i - 1];
      if (!ConvertNode(parameter_type_node, expected_parameter_type))
      {
        return nullptr;
      }
      // Need to check here that parameter_type does in fact support any operator(s)
      // required by the template_type's i'th type parameter...
      template_parameter_types.push_back(std::move(parameter_type));
    }
    if (num_user_defined_instantiation_types_++ >= MAX_USER_DEFINED_INSTANTIATION_TYPES)
    {
      throw FatalErrorException(filename_, identifier_node->line,
                                "maximum number of user defined instantiation types exceeded");
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

void Analyser::SetVariableExpression(ExpressionNodePtr const &node, VariablePtr const &variable,
                                     TypePtr const &owner)
{
  node->expression_kind = ExpressionKind::Variable;
  node->variable        = variable;
  node->type            = variable->type;
  node->owner           = owner;
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

void Analyser::SetFunctionGroupExpression(ExpressionNodePtr const &node,
                                          FunctionGroupPtr const & function_group,
                                          TypePtr const &          owner)
{
  node->expression_kind = ExpressionKind::FunctionGroup;
  node->function_group  = function_group;
  node->owner           = owner;
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
  FunctionPtr f = CreateFunction(FunctionKind::FreeFunction, name, unique_name, parameter_types, {},
                                 return_type);
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
  FunctionPtr f = CreateFunction(FunctionKind::Constructor, CONSTRUCTOR, unique_name,
                                 parameter_types, {}, type);
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
                                 parameter_types, {}, return_type);
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
                                 {}, return_type);
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

FunctionPtr Analyser::CreateUserDefinedConstructor(TypePtr const &         type,
                                                   TypePtrArray const &    parameter_types,
                                                   VariablePtrArray const &parameter_variables)
{
  std::string unique_name = BuildUniqueName(type, CONSTRUCTOR, parameter_types, type);
  return CreateFunction(FunctionKind::UserDefinedConstructor, CONSTRUCTOR, unique_name,
                        parameter_types, parameter_variables, type);
}

FunctionPtr Analyser::CreateUserDefinedMemberFunction(TypePtr const &type, std::string const &name,
                                                      TypePtrArray const &    parameter_types,
                                                      VariablePtrArray const &parameter_variables,
                                                      TypePtr const &         return_type)
{
  std::string unique_name = BuildUniqueName(type, name, parameter_types, return_type);
  return CreateFunction(FunctionKind::UserDefinedMemberFunction, name, unique_name, parameter_types,
                        parameter_variables, return_type);
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
                                  input_types, {}, output_type);
  AddFunctionInfo(gf, get_handler, get_static_charge);
  AddFunctionToSymbolTable(type->symbols, gf);
  function_map_.Add(gf);

  FunctionPtr sf = CreateFunction(FunctionKind::MemberFunction, SET_INDEXED_VALUE, s_unique_name,
                                  s_input_types, {}, void_type_);
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
    function_group = CreateFunctionGroup(function->name, nullptr);
    symbols->Add(function_group);
  }
  // Add the function to the function group
  function_group->functions.push_back(function);
}

}  // namespace vm
}  // namespace fetch
