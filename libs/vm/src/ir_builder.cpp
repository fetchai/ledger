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

#include "vm/ir_builder.hpp"

#include <memory>
#include <utility>

namespace fetch {
namespace vm {

void IRBuilder::Build(std::string const &name, BlockNodePtr const &root, IR &ir)
{
  ir_ = &ir;
  ir_->Reset();
  ir_->name_        = name;
  IRNodePtr ir_root = BuildNode(root);
  ir_->root_        = ConvertToIRBlockNodePtr(ir_root);
  ir_               = nullptr;
  type_map_.Clear();
  variable_map_.Clear();
  function_map_.Clear();
}

IRNodePtr IRBuilder::BuildNode(NodePtr const &node)
{
  if (node == nullptr)
  {
    return nullptr;
  }
  if (node->IsBasicNode())
  {
    IRNodePtr ir_node =
        CreateIRBasicNode(node->node_kind, node->text, node->line, BuildChildren(node->children));
    return ir_node;
  }
  if (node->IsBlockNode())
  {
    BlockNodePtr   block_node    = ConvertToBlockNodePtr(node);
    IRBlockNodePtr ir_block_node = CreateIRBlockNode(
        block_node->node_kind, block_node->text, block_node->line,
        BuildChildren(block_node->children), BuildChildren(block_node->block_children),
        block_node->block_terminator_text, block_node->block_terminator_line);
    return ir_block_node;
  }

  ExpressionNodePtr   expression_node    = ConvertToExpressionNodePtr(node);
  IRExpressionNodePtr ir_expression_node = CreateIRExpressionNode(
      expression_node->node_kind, expression_node->text, expression_node->line,
      BuildChildren(expression_node->children), expression_node->expression_kind,
      BuildType(expression_node->type), BuildVariable(expression_node->variable),
      BuildFunction(expression_node->function), BuildType(expression_node->owner));
  return ir_expression_node;
}

IRNodePtrArray IRBuilder::BuildChildren(NodePtrArray const &children)
{
  IRNodePtrArray ir_children;
  for (auto const &child : children)
  {
    ir_children.push_back(BuildNode(child));
  }
  return ir_children;
}

IRTypePtr IRBuilder::BuildType(TypePtr const &type)
{
  if (type == nullptr)
  {
    return nullptr;
  }
  IRTypePtr ir_type = type_map_.Find(type);
  if (ir_type)
  {
    return ir_type;
  }
  // Create and add to map BEFORE dependencies
  ir_type = CreateIRType(type->type_kind, type->name);
  type_map_.AddPair(type, ir_type);
  // Dependencies...
  if (type->IsInstantiation())
  {
    ir_type->template_type            = BuildType(type->template_type);
    ir_type->template_parameter_types = BuildTypes(type->types);
  }
  // Add to list AFTER dependencies
  ir_->AddType(ir_type);
  return ir_type;
}

IRVariablePtr IRBuilder::BuildVariable(VariablePtr const &variable)
{
  if (variable == nullptr)
  {
    return nullptr;
  }
  IRVariablePtr ir_variable = variable_map_.Find(variable);
  if (ir_variable)
  {
    return ir_variable;
  }
  // Create and add to map BEFORE dependencies
  ir_variable = CreateIRVariable(variable->variable_kind, variable->name, variable->referenced);
  variable_map_.AddPair(variable, ir_variable);
  // Dependencies...
  ir_variable->type = BuildType(variable->type);
  // Add to list AFTER dependencies
  ir_->AddVariable(ir_variable);
  return ir_variable;
}

IRFunctionPtr IRBuilder::BuildFunction(FunctionPtr const &function)
{
  if (function == nullptr)
  {
    return nullptr;
  }
  IRFunctionPtr ir_function = function_map_.Find(function);
  if (ir_function)
  {
    return ir_function;
  }
  // Create and add to map BEFORE dependencies
  ir_function = CreateIRFunction(function->function_kind, function->name, function->unique_name);
  function_map_.AddPair(function, ir_function);
  // Dependencies...
  ir_function->parameter_types     = BuildTypes(function->parameter_types);
  ir_function->parameter_variables = BuildVariables(function->parameter_variables);
  ir_function->return_type         = BuildType(function->return_type);
  // Add to list AFTER dependencies
  ir_->AddFunction(ir_function);
  return ir_function;
}

IRTypePtrArray IRBuilder::BuildTypes(TypePtrArray const &types)
{
  IRTypePtrArray array;
  array.reserve(types.size());
  for (auto const &type : types)
  {
    array.push_back(BuildType(type));
  }
  return array;
}

IRFunctionPtrArray IRBuilder::BuildFunctions(FunctionPtrArray const &functions)
{
  IRFunctionPtrArray array;
  array.reserve(functions.size());
  for (auto const &function : functions)
  {
    array.push_back(BuildFunction(function));
  }
  return array;
}

IRVariablePtrArray IRBuilder::BuildVariables(VariablePtrArray const &variables)
{
  IRVariablePtrArray array;
  array.reserve(variables.size());
  for (auto const &variable : variables)
  {
    array.push_back(BuildVariable(variable));
  }
  return array;
}

}  // namespace vm
}  // namespace fetch
