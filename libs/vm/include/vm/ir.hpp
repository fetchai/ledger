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
         IRTypePtrArray template_parameter_types__)
    : type_kind{type_kind__}
    , name{std::move(name__)}
    , template_type{std::move(template_type__)}
    , template_parameter_types{std::move(template_parameter_types__)}
  {}
  virtual ~IRType() = default;

  virtual void Reset()
  {
    template_type = nullptr;
    template_parameter_types.clear();
  }

  bool IsVoid() const
  {
    return (name == "Void");
  }
  bool IsPrimitive() const
  {
    return (type_kind == TypeKind::Primitive);
  }
  bool IsClass() const
  {
    return (type_kind == TypeKind::Class);
  }
  bool IsTemplate() const
  {
    return (type_kind == TypeKind::Template);
  }
  bool IsTemplateInstantiation() const
  {
    return (type_kind == TypeKind::TemplateInstantiation);
  }
  bool IsUserDefinedTemplateInstantiation() const
  {
    return (type_kind == TypeKind::UserDefinedTemplateInstantiation);
  }
  bool IsInstantiation() const
  {
    return IsTemplateInstantiation() || IsUserDefinedTemplateInstantiation();
  }
  bool IsUserDefinedContract() const
  {
    return (type_kind == TypeKind::UserDefinedContract);
  }

  TypeKind       type_kind{TypeKind::Unknown};
  std::string    name;
  IRTypePtr      template_type;
  IRTypePtrArray template_parameter_types;
  // only used during code generation
  uint16_t id{};
};

inline IRTypePtr CreateIRType(TypeKind type_kind, std::string name, IRTypePtr template_type,
                              IRTypePtrArray template_parameter_types)
{
  return std::make_shared<IRType>(type_kind, std::move(name), std::move(template_type),
                                  std::move(template_parameter_types));
}

struct IRVariable
{
  IRVariable(VariableKind variable_kind__, std::string name__, IRTypePtr type__, bool referenced__)
    : variable_kind{variable_kind__}
    , name{std::move(name__)}
    , type{std::move(type__)}
    , referenced{referenced__}
  {}
  virtual ~IRVariable() = default;

  virtual void Reset()
  {
    type = nullptr;
  }

  VariableKind variable_kind{VariableKind::Unknown};
  std::string  name;
  IRTypePtr    type;
  bool         referenced;
  // only used during code generation
  uint16_t id{};
};
using IRVariablePtr      = std::shared_ptr<IRVariable>;
using IRVariablePtrArray = std::vector<IRVariablePtr>;

inline IRVariablePtr CreateIRVariable(VariableKind variable_kind, std::string name, IRTypePtr type,
                                      bool referenced)
{
  return std::make_shared<IRVariable>(variable_kind, std::move(name), std::move(type), referenced);
}

struct IRFunction
{
  IRFunction(FunctionKind function_kind__, std::string name__, std::string unique_name__,
             IRTypePtrArray parameter_types__, IRVariablePtrArray parameter_variables__,
             IRTypePtr return_type__)
    : function_kind{function_kind__}
    , name{std::move(name__)}
    , unique_name{std::move(unique_name__)}
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
  FunctionKind       function_kind{FunctionKind::Unknown};
  std::string        name;
  std::string        unique_name;
  IRTypePtrArray     parameter_types;
  IRVariablePtrArray parameter_variables;
  IRTypePtr          return_type;
  // only used during code generation
  uint16_t id{};
};
using IRFunctionPtr      = std::shared_ptr<IRFunction>;
using IRFunctionPtrArray = std::vector<IRFunctionPtr>;

inline IRFunctionPtr CreateIRFunction(FunctionKind function_kind, std::string name,
                                      std::string unique_name, IRTypePtrArray parameter_types,
                                      IRVariablePtrArray parameter_variables, IRTypePtr return_type)
{
  return std::make_shared<IRFunction>(function_kind, std::move(name), std::move(unique_name),
                                      std::move(parameter_types), std::move(parameter_variables),
                                      std::move(return_type));
}

struct IRNode;
using IRNodePtr      = std::shared_ptr<IRNode>;
using IRNodePtrArray = std::vector<IRNodePtr>;
struct IRNode
{
  IRNode(NodeCategory node_category__, NodeKind node_kind__, std::string text__, uint16_t line__,
         IRNodePtrArray children__)
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

  NodeCategory   node_category{NodeCategory::Unknown};
  NodeKind       node_kind{NodeKind::Unknown};
  std::string    text;
  uint16_t       line{};
  IRNodePtrArray children;
};

inline IRNodePtr CreateIRBasicNode(NodeKind node_kind, std::string text, uint16_t line,
                                   IRNodePtrArray children)
{
  return std::make_shared<IRNode>(NodeCategory::Basic, node_kind, std::move(text), line,
                                  std::move(children));
}

struct IRBlockNode : public IRNode
{
  IRBlockNode(NodeKind node_kind, std::string text, uint16_t line, IRNodePtrArray children,
              IRNodePtrArray block_children__, std::string block_terminator_text__,
              uint16_t block_terminator_line__)
    : IRNode(NodeCategory::Block, node_kind, std::move(text), line, std::move(children))
    , block_children{std::move(block_children__)}
    , block_terminator_text{std::move(block_terminator_text__)}
    , block_terminator_line{block_terminator_line__}
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
  uint16_t       block_terminator_line{};
};
using IRBlockNodePtr      = std::shared_ptr<IRBlockNode>;
using IRBlockNodePtrArray = std::vector<IRBlockNodePtr>;

inline IRBlockNodePtr CreateIRBlockNode(NodeKind node_kind, std::string text, uint16_t line,
                                        IRNodePtrArray children, IRNodePtrArray block_children,
                                        std::string block_terminator_text,
                                        uint16_t    block_terminator_line)
{
  return std::make_shared<IRBlockNode>(node_kind, std::move(text), line, std::move(children),
                                       std::move(block_children), std::move(block_terminator_text),
                                       block_terminator_line);
}

struct IRExpressionNode : public IRNode
{
  IRExpressionNode(NodeKind node_kind, std::string text, uint16_t line, IRNodePtrArray children,
                   ExpressionKind expression_kind__, IRTypePtr type__, IRVariablePtr variable__,
                   bool function_invoker_is_instance__, IRFunctionPtr function__)
    : IRNode(NodeCategory::Expression, node_kind, std::move(text), line, std::move(children))
    , expression_kind{expression_kind__}
    , type{std::move(type__)}
    , variable{std::move(variable__)}
    , function_invoker_is_instance{function_invoker_is_instance__}
    , function{std::move(function__)}
  {}
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

  ExpressionKind expression_kind{ExpressionKind::Unknown};
  IRTypePtr      type;
  IRVariablePtr  variable;
  bool           function_invoker_is_instance;
  IRFunctionPtr  function;
};
using IRExpressionNodePtr      = std::shared_ptr<IRExpressionNode>;
using IRExpressionNodePtrArray = std::vector<IRExpressionNodePtr>;

inline IRExpressionNodePtr CreateIRExpressionNode(NodeKind node_kind, std::string text,
                                                  uint16_t line, IRNodePtrArray children,
                                                  ExpressionKind expression_kind, IRTypePtr type,
                                                  IRVariablePtr variable,
                                                  bool          function_invoker_is_instance,
                                                  IRFunctionPtr function)
{
  return std::make_shared<IRExpressionNode>(node_kind, std::move(text), line, std::move(children),
                                            expression_kind, std::move(type), std::move(variable),
                                            function_invoker_is_instance, std::move(function));
}

inline IRBlockNodePtr ConvertToIRBlockNodePtr(IRNodePtr const &node)
{
  assert(!node || node->IsBlockNode());
  return std::static_pointer_cast<IRBlockNode>(node);
}

inline IRExpressionNodePtr ConvertToIRExpressionNodePtr(IRNodePtr const &node)
{
  assert(!node || node->IsExpressionNode());
  return std::static_pointer_cast<IRExpressionNode>(node);
}

class IR
{
public:
  IR() = default;
  ~IR();
  IR(IR const &other);
  IR &operator=(IR const &other);
  IR(IR &&other) noexcept;
  IR &operator=(IR &&other) noexcept;

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
  IRTypePtrArray     CloneTypes(IRTypePtrArray const &types);
  IRVariablePtrArray CloneVariables(IRVariablePtrArray const &variables);

  void AddType(IRTypePtr const &type)
  {
    types_.push_back(type);
  }

  void AddVariable(IRVariablePtr const &variable)
  {
    variables_.push_back(variable);
  }

  void AddFunction(IRFunctionPtr const &function)
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
