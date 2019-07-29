#pragma once
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

#include "vm/common.hpp"
#include "vm/opcodes.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fetch {
namespace vm {

struct IRType;
using IRTypePtr      = std::shared_ptr<IRType>;
using IRTypePtrArray = std::vector<IRTypePtr>;
struct IRType
{
  IRType(TypeKind type_kind__, std::string const &name__, IRTypePtr const &template_type__,
         IRTypePtrArray const &parameter_types__)
  {
    type_kind       = type_kind__;
    name            = name__;
    template_type   = template_type__;
    parameter_types = parameter_types__;
    resolved_id     = TypeIds::Unknown;
  }
  virtual ~IRType() = default;
  virtual void Reset()
  {
    template_type = nullptr;
    parameter_types.clear();
  }
  bool IsNull() const
  {
    return (name == "Null");
  }
  bool IsVoid() const
  {
    return (name == "Void");
  }
  bool IsPrimitive() const
  {
    return (type_kind == TypeKind::Primitive);
  }
  bool IsInstantiation() const
  {
    return ((type_kind == TypeKind::Instantiation) ||
            (type_kind == TypeKind::UserDefinedInstantiation));
  }
  TypeKind       type_kind;
  std::string    name;
  IRTypePtr      template_type;
  IRTypePtrArray parameter_types;
  uint16_t       resolved_id;
};

inline IRTypePtr CreateIRType(TypeKind type_kind, std::string const &name,
                              IRTypePtr const &template_type, IRTypePtrArray const &parameter_types)
{
  return std::make_shared<IRType>(IRType(type_kind, name, template_type, parameter_types));
}

struct IRVariable
{
  IRVariable(VariableKind variable_kind__, std::string const &name__, IRTypePtr const &type__,
      bool referenced__)
  {
    variable_kind = variable_kind__;
    name          = name__;
    type          = type__;
    referenced    = referenced__;
    index         = 0;
  }
  virtual ~IRVariable() = default;
  virtual void Reset()
  {
    type = nullptr;
  }
  VariableKind variable_kind;
  std::string  name;
  IRTypePtr    type;
  bool         referenced;
  uint16_t     index;
};
using IRVariablePtr      = std::shared_ptr<IRVariable>;
using IRVariablePtrArray = std::vector<IRVariablePtr>;

inline IRVariablePtr CreateIRVariable(VariableKind variable_kind, std::string const &name,
                                      IRTypePtr const &type, bool referenced)
{
  return std::make_shared<IRVariable>(IRVariable(variable_kind, name, type, referenced));
}

struct IRFunction
{
  IRFunction(FunctionKind function_kind__, std::string const &name__,
             std::string const &unique_id__, IRTypePtrArray const &parameter_types__,
             IRVariablePtrArray const &parameter_variables__, IRTypePtr const &return_type__)
  {
    function_kind       = function_kind__;
    name                = name__;
    unique_id           = unique_id__;
    parameter_types     = parameter_types__;
    parameter_variables = parameter_variables__;
    return_type         = return_type__;
    index               = 0;
    resolved_opcode     = Opcodes::Unknown;
  }
  void Reset()
  {
    parameter_types.clear();
    parameter_variables.clear();
    return_type = nullptr;
  }
  FunctionKind       function_kind;
  std::string        name;
  std::string        unique_id;
  IRTypePtrArray     parameter_types;
  IRVariablePtrArray parameter_variables;
  IRTypePtr          return_type;
  uint16_t           index;
  uint16_t           resolved_opcode;
};
using IRFunctionPtr      = std::shared_ptr<IRFunction>;
using IRFunctionPtrArray = std::vector<IRFunctionPtr>;

inline IRFunctionPtr CreateIRFunction(FunctionKind function_kind, std::string const &name,
                                      std::string const &       unique_id,
                                      IRTypePtrArray const &    parameter_types,
                                      IRVariablePtrArray const &parameter_variables,
                                      IRTypePtr const &         return_type)
{
  return std::make_shared<IRFunction>(IRFunction(function_kind, name, unique_id, parameter_types,
                                                 parameter_variables, return_type));
}

struct IRNode;
using IRNodePtr      = std::shared_ptr<IRNode>;
using IRNodePtrArray = std::vector<IRNodePtr>;
struct IRNode
{
  IRNode(NodeCategory node_category__, NodeKind node_kind__, std::string const &text__,
         uint16_t line__, IRNodePtrArray const &children__)
  {
    node_category = node_category__;
    node_kind     = node_kind__;
    text          = text__;
    line          = line__;
    children      = children__;
  }
  virtual ~IRNode() = default;
  virtual void Reset()
  {
    for (auto &child : children)
    {
      if (child)
      {
        child->Reset();
      }
    }
  }
  bool IsBasicNode() const
  {
    return (node_category == NodeCategory::Basic);
  }
  bool IsBlockNode() const
  {
    return (node_category == NodeCategory::Block);
  }
  bool IsExpressionNode() const
  {
    return (node_category == NodeCategory::Expression);
  }
  NodeCategory   node_category;
  NodeKind       node_kind;
  std::string    text;
  uint16_t       line;
  IRNodePtrArray children;
};

inline IRNodePtr CreateIRBasicNode(NodeKind node_kind, std::string const &text, uint16_t line,
                                   IRNodePtrArray const &children)
{
  return std::make_shared<IRNode>(IRNode(NodeCategory::Basic, node_kind, text, line, children));
}

struct IRBlockNode : public IRNode
{
  IRBlockNode(NodeKind node_kind, std::string const &text, uint16_t line,
              IRNodePtrArray const &children)
    : IRNode(NodeCategory::Block, node_kind, text, line, children)
  {}
  ~IRBlockNode() override = default;

  void Reset() override
  {
    IRNode::Reset();
    for (auto &child : block_children)
    {
      child->Reset();
    }
  }
  IRNodePtrArray block_children;
  std::string    block_terminator_text;
  uint16_t       block_terminator_line;
};
using IRBlockNodePtr      = std::shared_ptr<IRBlockNode>;
using IRBlockNodePtrArray = std::vector<IRBlockNodePtr>;

inline IRBlockNodePtr CreateIRBlockNode(NodeKind node_kind, std::string const &text, uint16_t line,
                                        IRNodePtrArray const &children)
{
  return std::make_shared<IRBlockNode>(IRBlockNode(node_kind, text, line, children));
}

struct IRExpressionNode : public IRNode
{
  IRExpressionNode(NodeKind node_kind, std::string const &text, uint16_t line,
                   IRNodePtrArray const &children)
    : IRNode(NodeCategory::Expression, node_kind, text, line, children)
  {
    expression_kind = ExpressionKind::Unknown;
  }
  ~IRExpressionNode() override = default;

  void Reset() override
  {
    IRNode::Reset();
    type     = nullptr;
    variable = nullptr;
    function = nullptr;
  }
  bool IsVariableExpression() const
  {
    return (expression_kind == ExpressionKind::Variable);
  }
  bool IsLVExpression() const
  {
    return (expression_kind == ExpressionKind::LV);
  }
  bool IsRVExpression() const
  {
    return (expression_kind == ExpressionKind::RV);
  }
  bool IsTypeExpression() const
  {
    return (expression_kind == ExpressionKind::Type);
  }
  bool IsFunctionGroupExpression() const
  {
    return (expression_kind == ExpressionKind::FunctionGroup);
  }
  ExpressionKind expression_kind;
  IRTypePtr      type;
  IRVariablePtr  variable;
  IRFunctionPtr  function;
};
using IRExpressionNodePtr      = std::shared_ptr<IRExpressionNode>;
using IRExpressionNodePtrArray = std::vector<IRExpressionNodePtr>;

inline IRExpressionNodePtr CreateIRExpressionNode(NodeKind node_kind, std::string const &text,
                                                  uint16_t line, IRNodePtrArray const &children)
{
  return std::make_shared<IRExpressionNode>(IRExpressionNode(node_kind, text, line, children));
}

inline IRBlockNodePtr ConvertToIRBlockNodePtr(IRNodePtr const &node)
{
  return std::static_pointer_cast<IRBlockNode>(node);
}

inline IRExpressionNodePtr ConvertToIRExpressionNodePtr(IRNodePtr const &node)
{
  return std::static_pointer_cast<IRExpressionNode>(node);
}

class IR
{
public:
  IR() = default;
  ~IR();
  IR(IR const &other);
  IR &operator=(IR const &other);
  IR(IR &&other);
  IR &operator=(IR &&other);

private:
  template <typename Key, typename Value>
  struct Map
  {
    void AddPair(Key const &key, Value const &value)
    {
      map[key] = value;
    }
    Value Find(Key const &key) const
    {
      auto it = map.find(key);
      if (it != map.end())
      {
        return it->second;
      }
      return nullptr;
    }
    void Clear()
    {
      std::unordered_map<Key, Value>().swap(map);
    }
    std::unordered_map<Key, Value> map;
  };

  void               Reset();
  void               Clone(IR const &other);
  IRNodePtr          CloneNode(IRNodePtr const &node);
  IRNodePtrArray     CloneChildren(IRNodePtrArray const &children);
  IRTypePtr          CloneType(IRTypePtr const &type);
  IRVariablePtr      CloneVariable(IRVariablePtr const &variable);
  IRFunctionPtr      CloneFunction(IRFunctionPtr const &function);
  IRTypePtrArray     CloneTypes(const IRTypePtrArray &types);
  IRVariablePtrArray CloneVariables(const IRVariablePtrArray &variables);

  void AddType(const IRTypePtr &type)
  {
    types_.push_back(type);
  }

  void AddVariable(const IRVariablePtr &variable)
  {
    variables_.push_back(variable);
  }

  void AddFunction(const IRFunctionPtr &function)
  {
    functions_.push_back(function);
  }

  std::string                           name_;
  IRBlockNodePtr                        root_;
  IRTypePtrArray                        types_;
  IRVariablePtrArray                    variables_;
  IRFunctionPtrArray                    functions_;
  IR::Map<IRTypePtr, IRTypePtr>         type_map_;
  IR::Map<IRVariablePtr, IRVariablePtr> variable_map_;
  IR::Map<IRFunctionPtr, IRFunctionPtr> function_map_;

  friend struct IRBuilder;
  friend class Generator;
};

}  // namespace vm
}  // namespace fetch
