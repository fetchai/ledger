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
  IRType(TypeKind type_kind__, std::string name__, IRTypePtr template_type__,
         IRTypePtrArray parameter_types__)
    : type_kind{type_kind__}
    , name{std::move(name__)}
    , template_type{std::move(template_type__)}
    , parameter_types{std::move(parameter_types__)}
  {}
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
  uint16_t       resolved_id = TypeIds::Unknown;
};

inline IRTypePtr CreateIRType(TypeKind type_kind, std::string name,
                              IRTypePtr template_type, IRTypePtrArray parameter_types)
{
  return std::make_shared<IRType>(IRType(type_kind, std::move(name), std::move(template_type), std::move(parameter_types)));
}

struct IRVariable
{
  IRVariable(VariableKind variable_kind__, std::string name__, IRTypePtr type__)
    : variable_kind{variable_kind__}
    , name{std::move(name__)}
    , type{std::move(type__)}
  {}
  virtual ~IRVariable() = default;

  virtual void Reset()
  {
    type = nullptr;
  }

  VariableKind variable_kind;
  std::string  name;
  IRTypePtr    type;
  uint16_t     index = 0;
};
using IRVariablePtr      = std::shared_ptr<IRVariable>;
using IRVariablePtrArray = std::vector<IRVariablePtr>;

inline IRVariablePtr CreateIRVariable(VariableKind variable_kind, std::string name, IRTypePtr type)
{
  return std::make_shared<IRVariable>(IRVariable(variable_kind, std::move(name), std::move(type)));
}

struct IRFunction
{
  IRFunction(FunctionKind function_kind__, std::string name__, std::string unique_id__,
             IRTypePtrArray parameter_types__, IRVariablePtrArray parameter_variables__,
             IRTypePtr return_type__)
    : function_kind{function_kind__}
    , name{std::move(name__)}
    , unique_id{std::move(unique_id__)}
    , parameter_types{std::move(parameter_types__)}
    , parameter_variables{std::move(parameter_variables__)}
    , return_type{std::move(return_type__)}
  {}
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
  uint16_t           index           = 0;
  uint16_t           resolved_opcode = Opcodes::Unknown;
};
using IRFunctionPtr      = std::shared_ptr<IRFunction>;
using IRFunctionPtrArray = std::vector<IRFunctionPtr>;

inline IRFunctionPtr CreateIRFunction(FunctionKind function_kind, std::string name,
                                      std::string        unique_id,
                                      IRTypePtrArray     parameter_types,
                                      IRVariablePtrArray parameter_variables,
                                      IRTypePtr          return_type)
{
  return std::make_shared<IRFunction>(IRFunction(function_kind,
						 std::move(name), std::move(unique_id), std::move(parameter_types),
                                                 std::move(parameter_variables), std::move(return_type)));
}

struct IRNode;
using IRNodePtr      = std::shared_ptr<IRNode>;
using IRNodePtrArray = std::vector<IRNodePtr>;
struct IRNode
{
  IRNode(NodeCategory node_category__, NodeKind node_kind__, std::string text__,
         uint16_t line__, IRNodePtrArray children__)
    : node_category{node_category__}
    , node_kind{node_kind__}
    , text{std::move(text__)}
    , line{line__}
    , children{std::move(children__)}
  {}
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

inline IRNodePtr CreateIRBasicNode(NodeKind node_kind, std::string text, uint16_t line,
                                   IRNodePtrArray children)
{
  return std::make_shared<IRNode>(IRNode(NodeCategory::Basic, node_kind, std::move(text), line, std::move(children)));
}

struct IRBlockNode : public IRNode
{
  IRBlockNode(NodeKind node_kind, std::string text, uint16_t line,
              IRNodePtrArray children)
    : IRNode(NodeCategory::Block, node_kind, std::move(text), line, std::move(children))
  {}
  virtual ~IRBlockNode() = default;

  virtual void Reset() override
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

inline IRBlockNodePtr CreateIRBlockNode(NodeKind node_kind, std::string text, uint16_t line,
                                        IRNodePtrArray children)
{
  return std::make_shared<IRBlockNode>(IRBlockNode(node_kind, std::move(text), line, std::move(children)));
}

struct IRExpressionNode : public IRNode
{
  IRExpressionNode(NodeKind node_kind, std::string text, uint16_t line,
                   IRNodePtrArray children)
    : IRNode(NodeCategory::Expression, node_kind, std::move(text), line, std::move(children))
  {
    expression_kind = ExpressionKind::Unknown;
  }
  virtual ~IRExpressionNode() = default;

  virtual void Reset() override
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

inline IRExpressionNodePtr CreateIRExpressionNode(NodeKind node_kind, std::string text,
                                                  uint16_t line, IRNodePtrArray children)
{
  return std::make_shared<IRExpressionNode>(IRExpressionNode(node_kind, std::move(text), line, std::move(children)));
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
    void AddPair(Key key, Value value)
    {
      map[std::move(key)] = std::move(value);
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
      map.clear();
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
