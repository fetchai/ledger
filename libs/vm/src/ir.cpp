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

#include "vm/ir.hpp"

namespace fetch {
namespace vm {

IR::IR(IR const &other)
{
  Clone(other);
}

IR::IR(IR &&other) noexcept
{
  name_      = std::move(other.name_);
  root_      = std::move(other.root_);
  types_     = std::move(other.types_);
  variables_ = std::move(other.variables_);
  functions_ = std::move(other.functions_);
}

IR &IR::operator=(IR const &other)
{
  if (&other != this)
  {
    Reset();
    Clone(other);
  }
  return *this;
}

IR &IR::operator=(IR &&other) noexcept
{
  if (&other != this)
  {
    Reset();
    name_      = std::move(other.name_);
    root_      = std::move(other.root_);
    types_     = std::move(other.types_);
    variables_ = std::move(other.variables_);
    functions_ = std::move(other.functions_);
  }
  return *this;
}

IR::~IR()
{
  Reset();
}

void IR::Reset()
{
  if (root_)
  {
    root_->Reset();
    root_ = nullptr;
  }
  types_.clear();
  variables_.clear();
  functions_.clear();
}

void IR::Clone(IR const &other)
{
  name_                = other.name_;
  IRNodePtr clone_root = CloneNode(other.root_);
  root_                = ConvertToIRBlockNodePtr(clone_root);
  type_map_.Clear();
  variable_map_.Clear();
  function_map_.Clear();
}

IRNodePtr IR::CloneNode(IRNodePtr const &node)
{
  if (node == nullptr)
  {
    return nullptr;
  }
  if (node->IsBasicNode())
  {
    IRNodePtr clone_node =
        CreateIRBasicNode(node->node_kind, node->text, node->line, CloneChildren(node->children));
    return clone_node;
  }
  else if (node->IsBlockNode())
  {
    IRBlockNodePtr block_node = ConvertToIRBlockNodePtr(node);
    IRBlockNodePtr clone_block_node =
        CreateIRBlockNode(block_node->node_kind, block_node->text, block_node->line,
                          CloneChildren(block_node->children));
    clone_block_node->block_children        = CloneChildren(block_node->block_children);
    clone_block_node->block_terminator_text = block_node->block_terminator_text;
    clone_block_node->block_terminator_line = block_node->block_terminator_line;
    return clone_block_node;
  }
  else
  {
    IRExpressionNodePtr expression_node = ConvertToIRExpressionNodePtr(node);
    IRExpressionNodePtr clone_expression_node =
        CreateIRExpressionNode(expression_node->node_kind, expression_node->text,
                               expression_node->line, CloneChildren(expression_node->children));
    clone_expression_node->expression_kind = expression_node->expression_kind;
    clone_expression_node->type            = CloneType(expression_node->type);
    clone_expression_node->variable        = CloneVariable(expression_node->variable);
    clone_expression_node->function        = CloneFunction(expression_node->function);
    return clone_expression_node;
  }
}

IRNodePtrArray IR::CloneChildren(IRNodePtrArray const &children)
{
  IRNodePtrArray clone_children;
  for (auto const &child : children)
  {
    clone_children.push_back(CloneNode(child));
  }
  return clone_children;
}

IRTypePtr IR::CloneType(IRTypePtr const &type)
{
  if (type == nullptr)
  {
    return nullptr;
  }
  IRTypePtr clone_type = type_map_.Find(type);
  if (clone_type)
  {
    return clone_type;
  }
  IRTypePtr      clone_template_type;
  IRTypePtrArray clone_parameter_types;
  if (type->IsInstantiation())
  {
    clone_template_type   = CloneType(type->template_type);
    clone_parameter_types = CloneTypes(type->parameter_types);
  }
  clone_type =
      CreateIRType(type->type_kind, type->name, clone_template_type, clone_parameter_types);
  type_map_.AddPair(type, clone_type);
  AddType(clone_type);
  return clone_type;
}

IRVariablePtr IR::CloneVariable(IRVariablePtr const &variable)
{
  if (variable == nullptr)
  {
    return nullptr;
  }
  IRVariablePtr clone_variable = variable_map_.Find(variable);
  if (clone_variable)
  {
    return clone_variable;
  }
  IRTypePtr clone_type = CloneType(variable->type);
  clone_variable =
      CreateIRVariable(variable->variable_kind, variable->name, clone_type, variable->referenced);
  variable_map_.AddPair(variable, clone_variable);
  AddVariable(clone_variable);
  return clone_variable;
}

IRFunctionPtr IR::CloneFunction(IRFunctionPtr const &function)
{
  if (function == nullptr)
  {
    return nullptr;
  }
  IRFunctionPtr clone_function = function_map_.Find(function);
  if (clone_function)
  {
    return clone_function;
  }
  IRTypePtrArray     clone_parameter_types     = CloneTypes(function->parameter_types);
  IRVariablePtrArray clone_parameter_variables = CloneVariables(function->parameter_variables);
  IRTypePtr          clone_return_type         = CloneType(function->return_type);
  clone_function =
      CreateIRFunction(function->function_kind, function->name, function->unique_id,
                       clone_parameter_types, clone_parameter_variables, clone_return_type);
  function_map_.AddPair(function, clone_function);
  AddFunction(clone_function);
  return clone_function;
}

IRTypePtrArray IR::CloneTypes(IRTypePtrArray const &types)
{
  IRTypePtrArray array;
  for (auto const &type : types)
  {
    array.push_back(CloneType(type));
  }
  return array;
}

IRVariablePtrArray IR::CloneVariables(IRVariablePtrArray const &variables)
{
  IRVariablePtrArray array;
  for (auto const &variable : variables)
  {
    array.push_back(CloneVariable(variable));
  }
  return array;
}

IRTypePtr CreateIRType(TypeKind type_kind, std::string name, IRTypePtr template_type,
                       IRTypePtrArray parameter_types)
{
  return std::make_shared<IRType>(type_kind, std::move(name), std::move(template_type),
                                  std::move(parameter_types));
}

IRVariablePtr CreateIRVariable(VariableKind variable_kind, std::string name, IRTypePtr type,
                               bool referenced)
{
  return std::make_shared<IRVariable>(variable_kind, std::move(name), std::move(type), referenced);
}

IRFunctionPtr CreateIRFunction(FunctionKind function_kind, std::string name, std::string unique_id,
                               IRTypePtrArray     parameter_types,
                               IRVariablePtrArray parameter_variables, IRTypePtr return_type)
{
  return std::make_shared<IRFunction>(function_kind, std::move(name), std::move(unique_id),
                                      std::move(parameter_types), std::move(parameter_variables),
                                      std::move(return_type));
}

IRNodePtr CreateIRBasicNode(NodeKind node_kind, std::string text, uint16_t line,
                            IRNodePtrArray children)
{
  return std::make_shared<IRNode>(NodeCategory::Basic, node_kind, std::move(text), line,
                                  std::move(children));
}

IRBlockNodePtr CreateIRBlockNode(NodeKind node_kind, std::string text, uint16_t line,
                                 IRNodePtrArray children)
{
  return std::make_shared<IRBlockNode>(node_kind, std::move(text), line, std::move(children));
}

IRExpressionNodePtr CreateIRExpressionNode(NodeKind node_kind, std::string text, uint16_t line,
                                           IRNodePtrArray children)
{
  return std::make_shared<IRExpressionNode>(node_kind, std::move(text), line, std::move(children));
}

IRBlockNodePtr ConvertToIRBlockNodePtr(IRNodePtr const &node)
{
  return std::static_pointer_cast<IRBlockNode>(node);
}

IRExpressionNodePtr ConvertToIRExpressionNodePtr(IRNodePtr const &node)
{
  return std::static_pointer_cast<IRExpressionNode>(node);
}

}  // namespace vm
}  // namespace fetch
