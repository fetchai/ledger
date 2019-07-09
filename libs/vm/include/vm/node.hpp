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

#include "meta/value_util.hpp"
#include "vm/common.hpp"
#include "vm/token.hpp"

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
  InplaceDivide      = 15,
  ArraySeq           = 16,
  ArrayRep           = 17
};

enum class SymbolKind : uint8_t
{
  Unknown       = 0,
  Type          = 1,
  Variable      = 2,
  FunctionGroup = 3
};

struct Symbol
{
  Symbol(SymbolKind symbol_kind__, std::string name)
    : symbol_kind{symbol_kind__}
    , name{std::move(name)}
  {}

  virtual ~Symbol() = default;

  virtual void Reset()
  {}

  constexpr bool IsType() const noexcept
  {
    return symbol_kind == SymbolKind::Type;
  }
  constexpr bool IsVariable() const noexcept
  {
    return symbol_kind == SymbolKind::Variable;
  }
  constexpr bool IsFunctionGroup() const noexcept
  {
    return symbol_kind == SymbolKind::FunctionGroup;
  }

  SymbolKind  symbol_kind;
  std::string name;
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
  return std::make_shared<SymbolTable>(SymbolTable());
}

struct Type;
using TypePtr      = std::shared_ptr<Type>;
using TypePtrArray = std::vector<TypePtr>;
using Operators    = std::unordered_set<Operator>;
struct Type : public Symbol
{
  Type(TypeKind type_kind, std::string name)
    : Symbol(SymbolKind::Type, std::move(name))
    , type_kind{type_kind}
  {}

  virtual ~Type() = default;

  virtual void Reset() override
  {
    if (symbols)
    {
      symbols->Reset();
    }
    template_type.reset();
    types.clear();
  }

  bool IsNull() const noexcept
  {
    return name == "Null";
  }
  bool IsVoid() const noexcept
  {
    return name == "Void";
  }
  constexpr bool IsPrimitive() const noexcept
  {
    return type_kind == TypeKind::Primitive;
  }
  constexpr bool IsMeta() const noexcept
  {
    return type_kind == TypeKind::Meta;
  }
  constexpr bool IsGroup() const noexcept
  {
    return type_kind == TypeKind::Group;
  }
  constexpr bool IsClass() const noexcept
  {
    return type_kind == TypeKind::Class;
  }
  constexpr bool IsInstantiation() const noexcept
  {
    return value_util::IsAnyOf(type_kind, TypeKind::Instantiation,
                               TypeKind::UserDefinedInstantiation);
  }

  TypeKind       type_kind;
  SymbolTablePtr symbols;
  TypePtr        template_type;
  TypePtrArray   types;
  Operators      ops;
  Operators      left_ops;
  Operators      right_ops;
  TypeId         id = TypeIds::Unknown;
};

inline TypePtr CreateType(TypeKind type_kind, std::string const &name)
{
  return std::make_shared<Type>(Type(type_kind, name));
}
inline TypePtr ConvertToTypePtr(SymbolPtr const &symbol)
{
  return std::static_pointer_cast<Type>(symbol);
}

struct Variable : public Symbol
{
  Variable(VariableKind variable_kind__, std::string const &name)
    : Symbol(SymbolKind::Variable, name)
  {
    variable_kind = variable_kind__;
  }
  virtual ~Variable() = default;
  virtual void Reset() override
  {
    type = nullptr;
  }
  VariableKind variable_kind;
  TypePtr      type;
};
using VariablePtr      = std::shared_ptr<Variable>;
using VariablePtrArray = std::vector<VariablePtr>;

inline VariablePtr CreateVariable(VariableKind variable_kind, std::string const &name)
{
  return std::make_shared<Variable>(Variable(variable_kind, name));
}
inline VariablePtr ConvertToVariablePtr(SymbolPtr const &symbol)
{
  return std::static_pointer_cast<Variable>(symbol);
}

struct Function
{
  Function(FunctionKind function_kind__, std::string const &name__, std::string const &unique_id__,
           TypePtrArray const &parameter_types__, VariablePtrArray const &parameter_variables__,
           TypePtr const &return_type__)
  {
    function_kind       = function_kind__;
    name                = name__;
    unique_id           = unique_id__;
    parameter_types     = parameter_types__;
    parameter_variables = parameter_variables__;
    return_type         = return_type__;
  }
  void Reset()
  {
    parameter_types.clear();
    parameter_variables.clear();
    return_type = nullptr;
  }
  FunctionKind     function_kind;
  std::string      name;
  std::string      unique_id;
  TypePtrArray     parameter_types;
  VariablePtrArray parameter_variables;
  TypePtr          return_type;
};
using FunctionPtr      = std::shared_ptr<Function>;
using FunctionPtrArray = std::vector<FunctionPtr>;

inline FunctionPtr CreateFunction(FunctionKind function_kind, std::string const &name,
                                  std::string const &unique_id, TypePtrArray const &parameter_types,
                                  VariablePtrArray const &parameter_variables,
                                  TypePtr const &         return_type)
{
  return std::make_shared<Function>(
      Function(function_kind, name, unique_id, parameter_types, parameter_variables, return_type));
}

struct FunctionGroup : public Symbol
{
  FunctionGroup(std::string const &name)
    : Symbol(SymbolKind::FunctionGroup, name)
  {}
  virtual ~FunctionGroup() = default;
  virtual void Reset() override
  {
    for (auto &function : functions)
    {
      function->Reset();
    }
  }
  FunctionPtrArray functions;
};
using FunctionGroupPtr      = std::shared_ptr<FunctionGroup>;
using FunctionGroupPtrArray = std::vector<FunctionGroupPtr>;

inline FunctionGroupPtr CreateFunctionGroup(std::string const &name)
{
  return std::make_shared<FunctionGroup>(FunctionGroup(name));
}
inline FunctionGroupPtr ConvertToFunctionGroupPtr(SymbolPtr const &symbol)
{
  return std::static_pointer_cast<FunctionGroup>(symbol);
}

struct Node;
using NodePtr      = std::shared_ptr<Node>;
using NodePtrArray = std::vector<NodePtr>;
struct Node
{
  Node(NodeCategory node_category, NodeKind node_kind, std::string text, uint16_t line)
    : node_category{node_category}
    , node_kind{node_kind}
    , text{std::move(text)}
    , line{line}
  {}

  Node(NodeCategory node_category, NodeKind node_kind, std::string text, uint16_t line,
       NodePtrArray children)
    : node_category{node_category}
    , node_kind{node_kind}
    , text{std::move(text)}
    , line{line}
    , children{std::move(children)}
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

  NodeCategory node_category;
  NodeKind     node_kind;
  std::string  text;
  uint16_t     line;
  NodePtrArray children;
};

inline NodePtr CreateBasicNode(NodeKind node_kind, std::string const &text, uint16_t line)
{
  return std::make_shared<Node>(Node(NodeCategory::Basic, node_kind, text, line));
}

struct BlockNode : public Node
{
  BlockNode(NodeKind node_kind, std::string text, uint16_t line)
    : Node(NodeCategory::Block, node_kind, std::move(text), line)
  {}

  virtual ~BlockNode() = default;

  virtual void Reset() override
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
  uint16_t       block_terminator_line = 0;
  SymbolTablePtr symbols;
};
using BlockNodePtr      = std::shared_ptr<BlockNode>;
using BlockNodePtrArray = std::vector<BlockNodePtr>;

inline BlockNodePtr CreateBlockNode(NodeKind node_kind, std::string text, uint16_t line)
{
  return std::make_shared<BlockNode>(BlockNode(node_kind, std::move(text), line));
}

struct ExpressionNode : public Node
{
  ExpressionNode(NodeKind node_kind, std::string text, uint16_t line)
    : Node(NodeCategory::Expression, node_kind, std::move(text), line)
  {}
  ExpressionNode(NodeKind node_kind, std::string text, uint16_t line, NodePtrArray children)
    : Node(NodeCategory::Expression, node_kind, std::move(text), line, std::move(children))
  {}
  virtual ~ExpressionNode() = default;

  virtual void Reset() override
  {
    Node::Reset();
    value_util::ZeroAll(type, variable, fg, function);
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

  ExpressionKind   expression_kind = ExpressionKind::Unknown;
  TypePtr          type;
  VariablePtr      variable;
  FunctionGroupPtr fg;
  bool             function_invoked_on_instance = false;
  FunctionPtr      function;
};
using ExpressionNodePtr = std::shared_ptr<ExpressionNode>;

inline ExpressionNodePtr CreateExpressionNode(NodeKind node_kind, std::string text, uint16_t line)
{
  return std::make_shared<ExpressionNode>(ExpressionNode(node_kind, std::move(text), line));
}

inline ExpressionNodePtr CreateExpressionNode(NodeKind node_kind, std::string text, uint16_t line,
                                              NodePtrArray children)
{
  return std::make_shared<ExpressionNode>(
      ExpressionNode(node_kind, std::move(text), line, std::move(children)));
}

inline BlockNodePtr ConvertToBlockNodePtr(NodePtr const &node)
{
  return std::static_pointer_cast<BlockNode>(node);
}

inline ExpressionNodePtr ConvertToExpressionNodePtr(NodePtr const &node)
{
  return std::static_pointer_cast<ExpressionNode>(node);
}

inline bool IsEmptyArray(ExpressionNodePtr const &node) noexcept
{
  return node->node_kind == NodeKind::ArraySeq && node->children.empty();
}

}  // namespace vm
}  // namespace fetch
