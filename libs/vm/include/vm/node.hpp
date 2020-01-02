#pragma once
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

#include "vm/common.hpp"
#include "vm/token.hpp"

#include <memory>
#include <vector>

namespace fetch {
namespace vm {

enum class Operator : uint8_t
{
  Unknown            = 0,
  Equal              = 1,
  NotEqual           = 2,
  LessThan           = 3,
  LessThanOrEqual    = 4,
  GreaterThan        = 5,
  GreaterThanOrEqual = 6,
  Negate             = 7,
  Add                = 8,
  Subtract           = 9,
  Multiply           = 10,
  Divide             = 11,
  InplaceAdd         = 12,
  InplaceSubtract    = 13,
  InplaceMultiply    = 14,
  InplaceDivide      = 15
};

enum class SymbolKind : uint8_t
{
  Unknown       = 0,
  Type          = 1,
  Variable      = 2,
  FunctionGroup = 3
};

struct Type;
using TypePtr      = std::shared_ptr<Type>;
using TypePtrArray = std::vector<TypePtr>;
using Operators    = std::unordered_set<Operator>;

struct Symbol
{
  Symbol(SymbolKind symbol_kind__, std::string name__, TypePtr user_defined_type__)
    : symbol_kind{symbol_kind__}
    , name{std::move(name__)}
    , user_defined_type{std::move(user_defined_type__)}
  {}
  virtual ~Symbol() = default;
  virtual void Reset()
  {
    user_defined_type = nullptr;
  }
  bool IsType() const
  {
    return (symbol_kind == SymbolKind::Type);
  }
  bool IsVariable() const
  {
    return (symbol_kind == SymbolKind::Variable);
  }
  bool IsFunctionGroup() const
  {
    return (symbol_kind == SymbolKind::FunctionGroup);
  }
  SymbolKind  symbol_kind{SymbolKind::Unknown};
  std::string name;
  TypePtr     user_defined_type;
};
using SymbolPtr = std::shared_ptr<Symbol>;

struct SymbolTable
{
  void Add(SymbolPtr const &symbol)
  {
    map[symbol->name] = symbol;
  }
  SymbolPtr Find(std::string const &name) const
  {
    auto it = map.find(name);
    if (it != map.end())
    {
      return it->second;
    }
    return nullptr;
  }
  void Reset()
  {
    for (auto &it : map)
    {
      it.second->Reset();
    }
  }
  std::unordered_map<std::string, SymbolPtr> map;
};
using SymbolTablePtr = std::shared_ptr<SymbolTable>;

inline SymbolTablePtr CreateSymbolTable()
{
  return std::make_shared<SymbolTable>();
}

struct Type : public Symbol
{
  Type(TypeKind type_kind__, std::string name)
    : Symbol(SymbolKind::Type, std::move(name), nullptr)
    , type_kind{type_kind__}
  {}
  ~Type() override = default;
  void Reset() override
  {
    Symbol::Reset();
    if (symbols)
    {
      symbols->Reset();
    }
    template_type = nullptr;
    types.clear();
  }
  bool IsVoid() const
  {
    return (name == "Void");
  }
  bool IsPrimitive() const
  {
    return (type_kind == TypeKind::Primitive);
  }
  bool IsMeta() const
  {
    return (type_kind == TypeKind::Meta);
  }
  bool IsGroup() const
  {
    return (type_kind == TypeKind::Group);
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
  bool IsUserDefinedStruct() const
  {
    return (type_kind == TypeKind::UserDefinedStruct);
  }
  TypeKind       type_kind{TypeKind::Unknown};
  SymbolTablePtr symbols;
  TypePtr        template_type;
  TypePtrArray   types;
  Operators      ops;
  Operators      left_ops;
  Operators      right_ops;
  uint16_t       num_functions{};
  uint16_t       num_variables{};
  TypeId         id{TypeIds::Unknown};
};

inline TypePtr CreateType(TypeKind type_kind, std::string name)
{
  return std::make_shared<Type>(type_kind, std::move(name));
}
inline TypePtr ConvertToTypePtr(SymbolPtr const &symbol)
{
  assert(!symbol || symbol->IsType());
  return std::static_pointer_cast<Type>(symbol);
}

struct Variable : public Symbol
{
  Variable(VariableKind variable_kind__, std::string name, TypePtr type__,
           TypePtr user_defined_type)
    : Symbol(SymbolKind::Variable, std::move(name), std::move(user_defined_type))
    , variable_kind{variable_kind__}
    , type{std::move(type__)}
  {}
  ~Variable() override = default;

  void Reset() override
  {
    Symbol::Reset();
    type = nullptr;
  }
  VariableKind variable_kind{VariableKind::Unknown};
  TypePtr      type;
  bool         referenced{false};
};
using VariablePtr      = std::shared_ptr<Variable>;
using VariablePtrArray = std::vector<VariablePtr>;

inline VariablePtr CreateVariable(VariableKind variable_kind, std::string name, TypePtr type,
                                  TypePtr user_defined_type)
{
  return std::make_shared<Variable>(variable_kind, std::move(name), std::move(type),
                                    std::move(user_defined_type));
}
inline VariablePtr ConvertToVariablePtr(SymbolPtr const &symbol)
{
  assert(!symbol || symbol->IsVariable());
  return std::static_pointer_cast<Variable>(symbol);
}

struct Function
{
  Function(FunctionKind function_kind__, std::string name__, std::string unique_name__,
           TypePtrArray parameter_types__, VariablePtrArray parameter_variables__,
           TypePtr return_type__)
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
  FunctionKind     function_kind{FunctionKind::Unknown};
  std::string      name;
  std::string      unique_name;
  TypePtrArray     parameter_types;
  VariablePtrArray parameter_variables;
  TypePtr          return_type;
  uint16_t         num_locals{};
};
using FunctionPtr      = std::shared_ptr<Function>;
using FunctionPtrArray = std::vector<FunctionPtr>;

inline FunctionPtr CreateFunction(FunctionKind function_kind, std::string name,
                                  std::string unique_name, TypePtrArray parameter_types,
                                  VariablePtrArray parameter_variables, TypePtr return_type)
{
  return std::make_shared<Function>(function_kind, std::move(name), std::move(unique_name),
                                    std::move(parameter_types), std::move(parameter_variables),
                                    std::move(return_type));
}

struct FunctionGroup : public Symbol
{
  FunctionGroup(std::string name, TypePtr user_defined_type)
    : Symbol(SymbolKind::FunctionGroup, std::move(name), std::move(user_defined_type))
  {}
  ~FunctionGroup() override = default;

  void Reset() override
  {
    Symbol::Reset();
    for (auto &function : functions)
    {
      function->Reset();
    }
  }
  FunctionPtrArray functions;
};
using FunctionGroupPtr      = std::shared_ptr<FunctionGroup>;
using FunctionGroupPtrArray = std::vector<FunctionGroupPtr>;

inline FunctionGroupPtr CreateFunctionGroup(std::string name, TypePtr user_defined_type)
{
  return std::make_shared<FunctionGroup>(std::move(name), std::move(user_defined_type));
}
inline FunctionGroupPtr ConvertToFunctionGroupPtr(SymbolPtr const &symbol)
{
  assert(!symbol || symbol->IsFunctionGroup());
  return std::static_pointer_cast<FunctionGroup>(symbol);
}

struct Node;
using NodePtr      = std::shared_ptr<Node>;
using NodePtrArray = std::vector<NodePtr>;
struct Node
{
  Node(NodeCategory node_category__, NodeKind node_kind__, std::string text__, uint16_t line__)
    : node_category{node_category__}
    , node_kind{node_kind__}
    , text{std::move(text__)}
    , line{line__}
  {}
  virtual ~Node() = default;
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
  bool IsNull() const
  {
    return (node_kind == NodeKind::Null);
  }
  bool IsInitialiserList() const
  {
    return (node_kind == NodeKind::InitialiserList);
  }
  NodeCategory node_category{NodeCategory::Unknown};
  NodeKind     node_kind{NodeKind::Unknown};
  std::string  text;
  uint16_t     line{};
  NodePtrArray children;
};

inline NodePtr CreateBasicNode(NodeKind node_kind, std::string text, uint16_t line)
{
  return std::make_shared<Node>(NodeCategory::Basic, node_kind, std::move(text), line);
}

struct BlockNode : public Node
{
  BlockNode(NodeKind node_kind, std::string text, uint16_t line)
    : Node(NodeCategory::Block, node_kind, std::move(text), line)
  {}
  ~BlockNode() override = default;

  void Reset() override
  {
    Node::Reset();
    for (auto &child : block_children)
    {
      child->Reset();
    }
    if (symbols)
    {
      symbols->Reset();
    }
  }
  NodePtrArray   block_children;
  std::string    block_terminator_text;
  uint16_t       block_terminator_line{};
  SymbolTablePtr symbols;
};
using BlockNodePtr      = std::shared_ptr<BlockNode>;
using BlockNodePtrArray = std::vector<BlockNodePtr>;

inline BlockNodePtr CreateBlockNode(NodeKind node_kind, std::string text, uint16_t line)
{
  return std::make_shared<BlockNode>(node_kind, std::move(text), line);
}

struct ExpressionNode : public Node
{
  ExpressionNode(NodeKind node_kind, std::string text, uint16_t line)
    : Node(NodeCategory::Expression, node_kind, std::move(text), line)
  {}
  ~ExpressionNode() override = default;

  void Reset() override
  {
    Node::Reset();
    type           = nullptr;
    variable       = nullptr;
    function_group = nullptr;
    owner          = nullptr;
    function       = nullptr;
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
  bool IsConcrete() const
  {
    if (IsNull() || IsInitialiserList() || type->IsVoid())
    {
      return false;
    }
    return type->IsPrimitive() || type->IsClass() || type->IsInstantiation() ||
           type->IsUserDefinedStruct();
  }
  ExpressionKind   expression_kind{ExpressionKind::Unknown};
  TypePtr          type;
  VariablePtr      variable;
  FunctionGroupPtr function_group;
  TypePtr          owner;
  FunctionPtr      function;
};
using ExpressionNodePtr      = std::shared_ptr<ExpressionNode>;
using ExpressionNodePtrArray = std::vector<ExpressionNodePtr>;

inline ExpressionNodePtr CreateExpressionNode(NodeKind node_kind, std::string text, uint16_t line)
{
  return std::make_shared<ExpressionNode>(node_kind, std::move(text), line);
}

inline BlockNodePtr ConvertToBlockNodePtr(NodePtr const &node)
{
  assert(!node || node->IsBlockNode());
  return std::static_pointer_cast<BlockNode>(node);
}

inline ExpressionNodePtr ConvertToExpressionNodePtr(NodePtr const &node)
{
  assert(!node || node->IsExpressionNode());
  return std::static_pointer_cast<ExpressionNode>(node);
}

}  // namespace vm
}  // namespace fetch
